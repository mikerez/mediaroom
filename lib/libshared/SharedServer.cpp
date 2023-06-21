#include "SharedServer.h"

#include "sys/ioctl.h"
#include <cstring>

const size_t SharedCtx::default_shmem_size = 1024 * 1024 * 100;

SharedServer::SharedServer(): _max_sd(SOCKET_ERROR), _work(false), _shm_index(0), _curr_handling_sock(SOCKET_ERROR)
{
}

SharedServer::~SharedServer()
{
    _work = false;
    _ctx_map_sock.clear();
}

bool SharedServer::create()
{
    int enable = 1;
    _sock = socket(AF_INET, SOCK_STREAM, 0);
    if(_sock < 0)
    {
        LOG_ERR(DEBUG_SHARED_SERVER, "Failed to create server socket!\n");
        return false;
    }

    auto rc = setsockopt(_sock, SOL_SOCKET, SO_REUSEADDR, (const uint8_t *)&enable, sizeof (enable));
    if(rc < 0)
    {
        LOG_ERR(DEBUG_SHARED_SERVER,"Failed to setup socketopt SO_REUSEADDR!\n");
        closeSocket();
        return false;
    }

    rc = ioctl(_sock, FIONBIO, (void *)&enable);
    if(rc < 0)
    {
        LOG_ERR(DEBUG_SHARED_SERVER,"Failed to setup socket nonblocked!\n");
        closeSocket();
        return false;
    }

    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof (addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port        = htons(BASE_PORT);

    rc = bind(_sock, (struct sockaddr *)&addr, sizeof (addr));
    if(rc < 0)
    {
        LOG_ERR(DEBUG_SHARED_SERVER,"Failed to bind socket!\n");
        closeSocket();
        return false;
    }

    rc = listen(_sock, 5);
    if(rc < 0)
    {
        LOG_ERR(DEBUG_SHARED_SERVER,"Failed to listen socket!\n");
        closeSocket();
        return false;
    }

    FD_ZERO(&_main_set);
    _max_sd = _sock;
    FD_SET(_sock, &_main_set);

    _handler_thread = std::thread(&SharedServer::start, this);
    _handler_thread.detach();

    return true;
}

void SharedServer::start()
{
    _work = true;
    do
    {
        std::memcpy(&_work_set, &_main_set, sizeof (fd_set));
        auto rc = select(_max_sd + 1, &_work_set, nullptr, nullptr, nullptr);
        if(rc < 0)
        {
            LOG_ERR(DEBUG_SHARED_SERVER, "Select failed %i!\n", rc);
            closeSocket();
            break;

        }
        else if(rc == 0)
        {
            LOG_ERR(DEBUG_SHARED_SERVER, "Select timeout!\n");
        }

        auto num_of_ready_sd = rc;
        for(int i =0 ; i <= _max_sd && num_of_ready_sd > 0; i++)
        {
            if(FD_ISSET(i, &_work_set))
            {
                num_of_ready_sd--;
                // This is server socket. Accept all
                if(i == _sock)
                {
                    if(!acceptConnections())
                    {
                        LOG_ERR(DEBUG_SHARED_SERVER, "Error during accept new connections! Closing listen socket...\n");
                        closeSocket();
                        stop();
                        break;
                    }
                }
                else
                {
                    auto it_pd = _partial_data.find(i);
                    if(it_pd == _partial_data.end()) {
                        it_pd = _partial_data.insert({i, data_size_pair_t()}).first;
                    }

                    auto read = SharedSocket::read(i, it_pd->second.first.data() + it_pd->second.second, sizeof (ss_cmd) - it_pd->second.second);
                    if(read == SOCKET_ERROR)
                    {
                        closeConnection(i);
                    }
                    else
                    {
                        if(it_pd->second.second + read == sizeof (ss_cmd))
                        {
                            LOG_MESS(DEBUG_SHARED_SERVER, "[%i]: Got %i bytes. Buffer complete. Process cmd...\n", i, read);
                            _curr_handling_sock = i;
                            processCommands(*(struct ss_cmd *)it_pd->second.first.data());
                            it_pd->second.second = 0;
                            _curr_handling_sock = SOCKET_ERROR;
                        }
                        else
                        {
                            it_pd->second.second += read;
                            LOG_MESS(DEBUG_SHARED_SERVER, "[%i]: Got %i bytes. Buffer size %lu\n", i, read, it_pd->second.second);
                        }
                    }
                }
            }
        }
    } while(_work);
}

void SharedServer::processCommands(ss_cmd cmd)
{
    if(_curr_handling_sock == SOCKET_ERROR)
    {
        return;
    }

    auto socket_ctx_it = _socket_ctx_map.find(_curr_handling_sock);
    if((ss_proto)cmd.cmd == ss_proto::SS_INIT)
    {
        if(strcmp(cmd.text, INIT_KEYWORD) == 0)
        {
            socket_ctx_it->second.setState(SocketCtx::ss_state::SS_INITED);

            ss_cmd ack = { (uint64_t)ss_proto::SS_ACK };
            writeData(_curr_handling_sock, (uint8_t *)&ack, sizeof (ss_cmd));
        }
        else
        {
            // Spam connection
            closeConnection(_curr_handling_sock);
        }
        return;
    }

    if(!socket_ctx_it->second.inited())
    {
        return;
    }

    switch ((ss_proto)cmd.cmd) {
    case ss_proto::SS_INIT:
    {
        break;
    }
    case ss_proto::SS_NEWRCHAN:
    {
        newRChan(std::string(cmd.text + sizeof (uint32_t)), *(uint32_t *)(cmd.text));
        break;
    }
    case ss_proto::SS_NEWWCHAN:
    {
        newWChan(std::string(cmd.text + sizeof (uint32_t)), *(uint32_t *)(cmd.text));
        break;
    }
    case ss_proto::SS_BINDCHAN:
    {
        hash_t hash;
        std::memcpy(&hash, cmd.text, sizeof (hash_t));
        bindChan(cmd.id, hash);
        break;
    }
    case ss_proto::SS_CLOSECHAN:
    {
        closeChan(cmd.id);
        break;
    }
    default:
        LOG_ERR(DEBUG_SHARED_SERVER, "Unsupported cmd %lu on sock %i!\n", cmd.cmd, _curr_handling_sock);
        closeConnection(_curr_handling_sock);
        break;
    }
}

bool SharedServer::bindChan(const shm_id_t &id, const hash_t & hash)
{
    // ss_cmd::text containts hash for packets
    auto results = _ctx_map_sock.equal_range(_curr_handling_sock);
    if(results.first == results.second)
    {
        ss_cmd ack = { (uint64_t)ss_proto::SS_NACK };
        writeData(_curr_handling_sock, (uint8_t *)&ack, sizeof (ss_cmd));
    }

    bool sh_ctx_found = false;
    for(auto it = results.first; it != results.second; it++)
    {
        if(it->second->getId() == id)
        {
            sh_ctx_found = true;
            // TODO: Can shmem have multiple conditions???
            auto hash_ctx_map_it = _ctx_map_hash.find(hash);
            if(hash_ctx_map_it == _ctx_map_hash.end())
            {
                _ctx_map_hash.emplace(hash, it->second);
                _shmid_hash_map.emplace(id, std::vector<hash_t>() = { hash });

                ss_cmd ack = { (uint64_t)ss_proto::SS_ACK, id };
                writeData(_curr_handling_sock, (uint8_t *)&ack, sizeof (ss_cmd));
                break;
            }
            else
            {
                ss_cmd ack = { (uint64_t)ss_proto::SS_NACK, id };
                writeData(_curr_handling_sock, (uint8_t *)&ack, sizeof (ss_cmd));
                break;
            }
        }
    }

    if(!sh_ctx_found)
    {
        ss_cmd ack = { (uint64_t)ss_proto::SS_NACK };
        writeData(_curr_handling_sock, (uint8_t *)&ack, sizeof (ss_cmd));
        return false;
    }
    return true;
}

void SharedServer::stop()
{
    _work = false;
}

bool SharedServer::send(const hash_t &hash, uint8_t *data, const size_t &size)
{
    auto it = _ctx_map_hash.find(hash);
    if(it != _ctx_map_hash.end() && it->second->getDirection() == ss_direction::SS_WRITE)
    {
        return it->second->push(data, size);
    }

    return false;
}

bool SharedServer::hasClients() const
{
    return !_socket_ctx_map.empty();
}

bool SharedServer::newRChan(const std::string & prefix, const size_t & shm_size)
{
    auto id = getShmemIndex();
    std::string name = "/dev/shm/" + prefix + "_r_" + std::to_string(id);

    try {
        // NOTE: Invert direction for server side
        auto new_ctx = std::make_shared<SharedCtx>(name, id, ss_direction::SS_WRITE, shm_size ? shm_size : SharedCtx::default_shmem_size);
        _ctx_map_sock.emplace(_curr_handling_sock, new_ctx);

        ss_cmd ack = { (uint64_t)ss_proto::SS_ACK, id };
        std::memcpy(ack.text, name.c_str(), name.size());
        ack.text[name.size()] = '\0';
        writeData(_curr_handling_sock, (uint8_t *)&ack, sizeof (ss_cmd));
    }  catch (std::exception &e) {
        ss_cmd ack = { (uint64_t)ss_proto::SS_NACK };
        writeData(_curr_handling_sock, (uint8_t *)&ack, sizeof (ss_cmd));


        LOG_ERR(DEBUG_SHARED_SERVER, "[%i]: Failed to create SharedCtx(READ) with name %s and size %li! Err: %s\n", _curr_handling_sock, name.c_str(), shm_size, e.what());
        return false;
    }
    return true;
}

bool SharedServer::newWChan(const std::string & prefix, const size_t & shm_size)
{
    auto id = getShmemIndex();
    std::string name = "/dev/shm/" + prefix + "_w_" + std::to_string(id);

    try {
        auto new_ctx = std::make_shared<SharedCtx>(name, id, ss_direction::SS_READ, shm_size ? shm_size : SharedCtx::default_shmem_size);
        _ctx_map_sock.emplace(_curr_handling_sock, new_ctx);

        ss_cmd ack = { (uint64_t)ss_proto::SS_ACK, id };
        std::memcpy(ack.text, name.c_str(), name.size());
        ack.text[name.size()] = '\0';
        writeData(_curr_handling_sock, (uint8_t *)&ack, sizeof (ss_cmd));
    }  catch (std::exception &e) {
        ss_cmd ack = { (uint64_t)ss_proto::SS_NACK };
        writeData(_curr_handling_sock, (uint8_t *)&ack, sizeof (ss_cmd));

        LOG_ERR(DEBUG_SHARED_SERVER, "[%i]: Failed to create SharedCtx(WRITE) with name %s and size %li! Err: %s\n", _curr_handling_sock, name.c_str(), shm_size, e.what());
        return false;
    }
    return true;
}

bool SharedServer::closeChan(const shm_id_t &id)
{
    auto results = _ctx_map_sock.equal_range(_curr_handling_sock);
    for(auto it = results.first; it != results.second; it++)
    {
        if(it->second->getId() == id)
        {
            _ctx_map_sock.erase(it);
            break;
        }
    }

    auto it = _shmid_hash_map.find(id);
    if(it != _shmid_hash_map.end())
    {
        for(auto & hash : it->second)
        {
            _ctx_map_hash.erase(hash);
        }
    }
    _shmid_hash_map.erase(id);

    ss_cmd ack = { (uint64_t)ss_proto::SS_ACK, id};
    writeData(_curr_handling_sock, (uint8_t *)&ack, sizeof (ss_cmd));
    return true;
}

bool SharedServer::acceptConnections()
{
    sock_desc_t new_sd = -1;
    do
    {
        new_sd = accept(_sock, NULL, NULL);
        if(new_sd < 0)
        {
            if(errno != EWOULDBLOCK)
            {
                LOG_ERR(DEBUG_SHARED_SERVER, "Failed to accept new connection %s(%i).\n", strerror(errno), errno);
                return false;
            }
            break;
        }

        LOG_MESS(DEBUG_SHARED_SERVER, "Incomming connection %i.\n", new_sd);
        FD_SET(new_sd, &_main_set);
        if(new_sd > _max_sd)
        {
            _max_sd = new_sd;
        }

        struct timeval timeout = {0, 10};
        setsockopt(new_sd, SOL_SOCKET, SO_RCVTIMEO, (const uint8_t *)&timeout, sizeof timeout);

        auto it = _socket_ctx_map.insert({new_sd, SocketCtx()}).first;
        it->second.setState(SocketCtx::ss_state::SS_CONNECTED);

    } while(new_sd != SOCKET_ERROR);

    return true;
}

void SharedServer::closeConnection(sock_desc_t sd)
{
    _socket_ctx_map.erase(sd);
    _partial_data.erase(sd);
    close(sd);
    FD_CLR(sd, &_main_set);
    if(sd == _max_sd)
    {
        while (!FD_ISSET(_max_sd, &_main_set)) {
            _max_sd--;
        }
    }
}

void SharedServer::writeData(sock_desc_t sd, uint8_t *data, size_t size, bool close_on_error)
{
    auto written = SharedSocket::write(sd, data, size);
    if(written == SOCKET_ERROR)
    {
        LOG_ERR(DEBUG_SHARED_SERVER, "[%i]: Write error!\n", sd);
        if(close_on_error)
        {
            closeConnection(sd);
        }
    }
}

shm_id_t SharedServer::getShmemIndex()
{
    auto index = _shm_index;
    _shm_index++;
    return index;
}

SharedCtx::SharedCtx(const std::string &name, const shm_id_t & id, const ss_direction &side, const size_t & shm_size):
    _io(nullptr), _name(name), _id(id), _dir(side)
{
    _io = new SharedCycleBuffer(name.c_str(), shm_size, true);
}

SharedCtx::~SharedCtx()
{
    if(_io)
    {
        delete _io;
    }
}

shm_id_t SharedCtx::getId() const
{
    return _id;
}

ss_direction SharedCtx::getDirection() const
{
    return _dir;
}

bool SharedCtx::push(uint8_t *data, size_t size)
{
    if(_dir == ss_direction::SS_WRITE)
    {
        return (*_io)->write1(data, size);
    }
    else
    {
        return false;
    }
}

const uint8_t * SharedCtx::pop(size_t & size)
{
    if(_dir == ss_direction::SS_READ)
    {
        return (*_io)->read1(*(uint16_t *)&size);
    }
    else
    {
        size = 0;
        return nullptr;
    }
}

SocketCtx::SocketCtx(): _state(ss_state::SS_DISCONNECTED)
{

}

void SocketCtx::setState(const ss_state &state)
{
    _state = state;
}

bool SocketCtx::inited() const
{
    return _state == ss_state::SS_INITED;
}
