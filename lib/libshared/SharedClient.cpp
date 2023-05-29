#include "SharedClient.h"


SharedClient::SharedClient(const std::string &ip, const uint16_t &port)
{
    _cfg.ip = ip;
    _cfg.port = port;
}

SharedClient::~SharedClient()
{
}

bool SharedClient::create()
{
    _ip_addr.sin_family = AF_INET;
    _ip_addr.sin_port = htons(_cfg.port);

    if (inet_pton(AF_INET, _cfg.ip.c_str(), &_ip_addr.sin_addr) <= 0) {
        LOG_ERR(DEBUG_SHARED_CLIENT, "Cannot translate socket address: %s\n", _cfg.ip.c_str());
        return false;
    }

    return connectoToServer();
}

bool SharedClient::init()
{
    ss_cmd cmd = { (uint64_t)ss_proto::SS_INIT };
    std::memcpy(cmd.text, INIT_KEYWORD, sizeof (INIT_KEYWORD));
    return SharedSocket::write(_sock, (uint8_t *)&cmd, sizeof (ss_cmd));
}

bool SharedClient::newRChan(const std::string &prefix, const size_t &shm_size)
{
    ss_cmd cmd = { (uint64_t)ss_proto::SS_NEWRCHAN, 0 };

    size_t offset = 0;
    std::memcpy(cmd.text, &shm_size, sizeof (uint32_t));
    offset += sizeof (uint32_t);
    std::memcpy(cmd.text + offset, prefix.c_str(), prefix.size());

    return SharedSocket::write(_sock, (uint8_t *)&cmd, sizeof (ss_cmd));
}

bool SharedClient::newWChan(const std::string &prefix, const size_t &shm_size)
{
    ss_cmd cmd = { (uint64_t)ss_proto::SS_NEWWCHAN, 0 };

    size_t offset = 0;
    std::memcpy(cmd.text, &shm_size, sizeof (uint32_t));
    offset += sizeof (uint32_t);
    std::memcpy(cmd.text + offset, prefix.c_str(), prefix.size());

    return SharedSocket::write(_sock, (uint8_t *)&cmd, sizeof (ss_cmd));
}

bool SharedClient::bindChan(const uint64_t &id, const hash_t &hash)
{
    ss_cmd cmd = { (uint64_t)ss_proto::SS_BINDCHAN, id };
    std::memcpy(cmd.text, &hash, sizeof (hash_t));

    return SharedSocket::write(_sock, (uint8_t *)&cmd, sizeof (ss_cmd));
}

bool SharedClient::closeChan(const uint64_t &id)
{
    ss_cmd cmd = { (uint64_t)ss_proto::SS_CLOSECHAN, id };
    return SharedSocket::write(_sock, (uint8_t *)&cmd, sizeof (ss_cmd));
}

bool SharedClient::waitAck(ss_cmd & cmd)
{
    auto read = SharedSocket::read(_sock, (uint8_t *)&cmd, sizeof (ss_cmd));
    if(read != sizeof (ss_cmd))
    {
        closeSocket();
        return false;
    }
    return true;
}

bool SharedClient::connectoToServer()
{
    _sock = socket(AF_INET, SOCK_STREAM, 0);
    if (_sock < 0) {
        LOG_ERR(DEBUG_SHARED_CLIENT, "Cannot create socket ...\n");
        return false;
    }

    if(connect(_sock, (struct sockaddr *)&_ip_addr, sizeof(_ip_addr)) < 0)
    {
        LOG_MESS(DEBUG_SHARED_CLIENT, "Cannot connect to %s:%d. Err: %s(%i)\n", _cfg.ip.c_str(), _cfg.port, strerror(errno), errno);
        closeSocket();
        return false;
    }

    return true;
}

