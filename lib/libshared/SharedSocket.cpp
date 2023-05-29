#include "SharedSocket.h"

#include "sys/ioctl.h"

SharedSocket::SharedSocket(uint16_t port): _port(port), _sock(SOCKET_ERROR)
{
}

SharedSocket::~SharedSocket()
{
    closeSocket();
}

void SharedSocket::closeSocket()
{
    if(_sock != SOCKET_ERROR)
    {
        close(_sock);
        _sock = SOCKET_ERROR;
    }
}

int SharedSocket::read(sock_desc_t sock, uint8_t * data, size_t read_size)
{
    size_t read_bytes = 0;
    do
    {
        auto rc = recv(sock, data + read_bytes, read_size - read_bytes, 0);
        if(rc < 0)
        {
            if(errno != EWOULDBLOCK)
            {
                return SOCKET_ERROR;
            }
            break;
        }
        else if(rc == 0)
        {
            // Connection closed
            return SOCKET_ERROR;
        }
        else if(rc == read_size - read_bytes)
        {
            return read_size;
        }
        else if(rc < read_size - read_bytes)
        {
            read_bytes += rc;
        }

    } while (true);

    return read_bytes;
}

int SharedSocket::write(sock_desc_t sock, uint8_t *data, size_t size)
{
    if(!data || sock == SOCKET_ERROR)
    {
        return SOCKET_ERROR;
    }

    size_t total_sent = 0;
    while (total_sent != size)
    {
        auto sent = send(sock, data + total_sent, size - total_sent, 0);
        if (sent == -1)
        {
            return SOCKET_ERROR;
        }
        else if (sent == (ssize_t)(size - total_sent))
        {
            return sent;
        }
        else if (sent < (ssize_t)(size - total_sent))
        {  // this is not possible with blocking socket, probably one time before -1... if signal was got
            total_sent += (size_t)sent;
        }
    }

    return SOCKET_ERROR;
}
