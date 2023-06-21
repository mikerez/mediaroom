#include "SharedSocket.h"
#include "SharedCycleBuffer.h"


#include <map>
#include <unordered_map>
#include <thread>
#include <array>
#include <vector>

class SharedCtx
{
public:
    SharedCtx(const std::string &name, const shm_id_t &id, const ss_direction &side, const size_t & shm_size = default_shmem_size);
    ~SharedCtx();

    shm_id_t getId() const;
    ss_direction getDirection() const;

    bool push(uint8_t * data, size_t size);
    const uint8_t * pop(size_t & size);

    static const size_t default_shmem_size;
private:
    SharedCycleBuffer * _io;
    std::string _name;
    uint64_t _id;
    ss_direction _dir = ss_direction::SS_UNDEFINED;
};

class SocketCtx
{
public:
    SocketCtx();

    enum class ss_state
    {
        SS_DISCONNECTED,
        SS_CONNECTED,
        SS_INITED,
    };

    void setState(const ss_state & state);
    bool inited() const;
private:
    ss_state _state;
};

class SharedServer: public SharedSocket
{
  public:
    SharedServer();
    ~SharedServer();

    typedef std::multimap<sock_desc_t, std::shared_ptr<SharedCtx>> _ctx_map_sock_t;
    typedef std::unordered_map<hash_t, std::shared_ptr<SharedCtx>> _ctx_map_hash_t;

    bool create();
    void stop();
    bool send(const hash_t & hash, uint8_t * data, const size_t & size);

    // Tmp for test connection
    bool hasClients() const;
  private:
    // Contains all SharedCtx for special socked descriptor
    _ctx_map_sock_t _ctx_map_sock;
    /*
     * This map used to correlate hashes and shm id.
     * Need to remove all corresponds hashes if shared ctx with id will be removed;
     */
    std::unordered_map<shm_id_t, std::vector<hash_t>> _shmid_hash_map;
    // Correspond hash with SharedCtx
    _ctx_map_hash_t _ctx_map_hash;
    fd_set _main_set;
    fd_set _work_set;
    sock_desc_t _max_sd;
    bool _work;
    std::thread _handler_thread;
    shm_id_t _shm_index;
    sock_desc_t _curr_handling_sock;
    std::unordered_map<sock_desc_t, SocketCtx> _socket_ctx_map;

    // key: socket descriptor, value: data, size
    typedef std::pair<std::array<uint8_t, sizeof (ss_cmd)>, size_t> data_size_pair_t;
    std::unordered_map<sock_desc_t, data_size_pair_t> _partial_data;

    bool newRChan(const std::string & prefix, const size_t & shm_size);
    bool newWChan(const std::string & prefix, const size_t & shm_size);
    bool bindChan(const shm_id_t & id, const hash_t &hash);
    bool closeChan(const shm_id_t & id);

    void start();
    bool acceptConnections();
    void processCommands(ss_cmd cmd);

    void closeConnection(sock_desc_t sd);
    void writeData(sock_desc_t sd, uint8_t * data, size_t size, bool close_on_error = true);
    shm_id_t getShmemIndex();
};


