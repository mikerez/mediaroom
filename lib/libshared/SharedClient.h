#include "SharedSocket.h"

#include "netinet/in.h"

class SharedClient: public SharedSocket
{
public:
    SharedClient(const std::string & ip, const uint16_t & port);
    ~SharedClient();

    bool create();
    bool init();
    bool newRChan(const std::string &prefix, const size_t &shm_size);
    bool newWChan(const std::string &prefix, const size_t &shm_size);
    bool bindChan(const shm_id_t & id, const hash_t &hash);
    bool closeChan(const shm_id_t &id);
    bool waitAck(ss_cmd & cmd);
private:
    struct Config
    {
        std::string ip;
        uint16_t port;
    };
    Config _cfg;
    struct sockaddr_in _ip_addr;

    bool connectoToServer();
};


