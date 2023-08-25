#pragma once

#include <mutex>

#include "../core/Packet.h"
#include "../TimeHandler.h"

class Driver
{
public:
    Driver() {}
    virtual ~Driver() {};

    struct DriverStatistic {
        uint64_t rx_bytes = 0;
        uint64_t rx_packs = 0;
        uint64_t rx_drop  = 0;

        uint64_t rx_bytes_prev = 0;
        uint64_t rx_packs_prev = 0;
    };

    virtual size_t getPackets(Packet** pkts, size_t pkts_limix) = 0;
    virtual bool   finished() { return false; }
    virtual void   idle(unsigned id) { }

    std::mutex _lock;
};

class DriverOffline
{
    typedef TimeHandler::usecs_t time_t; // µs

public:
    Packet* offlinePkt() const
    {
        return _pkt;
    }

    bool offlineProcess(Packet * pkt, const timeval& pkt_ts)
    {
        RT_ASSERT(!_pkt);

        time_t cur_time = TimeHandler::Instance()->get_time_usecs();
        time_t pkt_time = TimeHandler::timeval_to_usecs(pkt_ts);
        time_t pkt_diff = (_pkt_time && pkt_time > _pkt_time) ? (pkt_time - _pkt_time) : 0;

        _pkt_time  = pkt_time;
        _pkt_sent += pkt_diff;

        if (_pkt_sent == 0)
            _pkt_sent = cur_time;
        if (_pkt_sent > cur_time)
            _pkt = pkt;
        return _pkt;
    }

    size_t offlineHandle(Packet** bulk)
    {
        RT_ASSERT(_pkt);

        time_t cur_time = TimeHandler::Instance()->get_time_usecs();
        if (_pkt_sent <= cur_time)
        {
            _pkt->update_time();
            *bulk = _pkt;
            _pkt = 0;
            return 1;
        }

        return 0;
    }

private:
    Packet * _pkt = 0;
    time_t   _pkt_time = 0; // in µs
    time_t   _pkt_sent = 0; // in µs
};
