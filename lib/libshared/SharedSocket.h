#pragma once

#include "SharedIO.h"

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <memory>

#include "../../src/core/Debug.h"

#define BASE_PORT           10105
#define SOCKET_ERROR        -1
#define INIT_KEYWORD        "hi!"

// basic methods for IO allocation when control socket is connected

enum class ss_proto
{
    SS_INIT,
    SS_ACK,
    SS_NACK,
    SS_NEWRCHAN,
    SS_NEWWCHAN,
    SS_BINDCHAN,
    SS_CLOSECHAN,
};

enum class ss_direction
{
    SS_UNDEFINED,
    SS_READ,
    SS_WRITE
};

struct ss_cmd
{
    uint64_t cmd;
    uint64_t id = 0;
    char text[128];
};

typedef int sock_desc_t;
typedef uint64_t hash_t;
typedef uint64_t shm_id_t;

class SharedSocket
{
public:
    SharedSocket(uint16_t port = BASE_PORT);
    virtual ~SharedSocket();

    virtual bool create() = 0;
    void closeSocket();
    virtual bool newRChan(const std::string & prefix, const size_t & shm_size) = 0;
    virtual bool newWChan(const std::string & prefix, const size_t & shm_size) = 0;
    virtual bool bindChan(const shm_id_t & id, const hash_t &hash) = 0;
    virtual bool closeChan(const shm_id_t & id) = 0;

    static int read(sock_desc_t sock, uint8_t *data, size_t read_size);
    static int write(sock_desc_t sock, uint8_t * data, size_t size);
protected:
    uint16_t _port;
    sock_desc_t _sock;
};


