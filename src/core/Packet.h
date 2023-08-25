// Copyright: https://github.com/mikerez/mediaroom/blob/main/LICENSE

#pragma once

#include <stdint.h>
#include <time.h>

#include "Os.h"
#include "PacketPtr.h"
#include "Eth.h"
#include "ss7/Mtp.h"
#include "ss7/Mtp3.h"
#include "ss7/Hdlc.h"
#include "Gfp.h"
#include "Config.h"
#include "PacketDetails.h"
#include "System.h"
#include "../TimeHandler.h"

#define MAX_PACKET_SIZE (65536/4)

struct Packet
{
    enum Type
    {
        L2Eth,
        L2Mtp,
        L2Mtp3,
        L2Hdlc,
        L2Gfp,
        L2Lapd
    };

    enum Proto  // only things that we probably need for sessions
    {
        Eth,  // broken or unknown Eth
        Mtp2,
        Mtp3,
        Hdlc,
        Gfp,
        Ip4,  // broken or unknown IP
        Tcp,
        Udp,
        Sctp,  // broken Sctp
        SctpChunk,  // broken or empty chunks
        M2ua,
        M3ua,
        Isup,
        Sccp,
        Tcap,
    };

    unsigned long long cpu_ticks;
    std::atomic<uint16_t> refcnt;
    uint16_t caplen;
    uint16_t chanid;
    uint16_t length;
    uint16_t offset;
    uint16_t payload_shift; // for TCP only
    Type type;
    Proto proto;
    Packet* next;
    Packet* last;

    void update_time()
    {
        cpu_ticks = rdtsc();
    }

    uint8_t* data()
    {
        return (uint8_t*)this + sizeof(Packet);
    }

    const uint8_t* data() const
    {
        return (const uint8_t*)this + sizeof(Packet);
    }

    uint16_t dataLength() const
    {
        return caplen;
    }

    void free() {
        if (--refcnt == 0) {
            if(next)
                next->free();
            ::free(this);
        }
    };

    PacketDetails* getDetails()
    {
        return (PacketDetails*)(data() + dataLength());
    }

    const PacketDetails* getDetails() const
    {
        return (const PacketDetails*)(data() + dataLength());
    }

    timeval ts() const {
        return TimeHandler::Instance()->get_time(cpu_ticks);
    }

    void addPacket(Packet* packet)
    {
        RT_ASSERT(packet != this);
        if(next) next->addPacket(packet);
        else     next = packet;
    }

    bool hasNext() const
    {
        return next;
    }

    Packet * getNext() {
        return next;
    }

    size_t chain_size() const
    {
        return 1 + (next ? next->chain_size() : 0);
    }

    size_t chain_data_size() const
    {
        return dataLength() + (next ? next->chain_data_size() : 0);
    }
};

inline Packet* allocPacket(size_t length)
{
    void* mem = malloc(sizeof(Packet)+length+CONFIG_PACKET_RESERVE);
    Packet* packet = new (mem) Packet();
    packet->update_time();
    packet->refcnt = 1;
    packet->next = nullptr;
    packet->last = nullptr;
    packet->payload_shift = 0;
    packet->caplen = static_cast<uint16_t>(length);  // required by next line
    packet->getDetails()->init();
    return packet;
}

inline Packet* reallocPacket(Packet* oldPacket, size_t length)
{
    void* mem = realloc(oldPacket, sizeof(Packet) + length + CONFIG_PACKET_RESERVE);
    Packet* packet = (Packet*)mem;
    packet->payload_shift = 0;
    packet->caplen = (uint16_t)length;  // required by next line
    packet->getDetails()->init();   // realloc clears details - use it only for testing purposes
    return packet;
}

struct PacketDeleter
{
    void operator()(Packet* pkt) {
        pkt->free();
    }
};

struct Pkt : public PacketPtr<std::unique_ptr<Packet,PacketDeleter>, Packet>
{
    typedef PacketPtr<std::unique_ptr<Packet,PacketDeleter>, Packet> SUBLAYER;
    typedef Pkt BaseType;

    Pkt()
    {}

    Pkt(SUBLAYER&& pkt) :
        SUBLAYER(move(pkt))
    {}

    Pkt& operator=(Pkt&& pkt)
    {
        setPtr(move(pkt.getPtr()));
        return *this;
    }

    Pkt(typename SUBLAYER::BasePtr&& pkt, uint16_t length, uint16_t offset = sizeof(Packet)) :
        SUBLAYER(move(pkt), length, offset)
    {}

    Pkt(uint16_t length, Packet* pkt, uint16_t offset = sizeof(Packet)) :
        SUBLAYER(move(std::unique_ptr<Packet, PacketDeleter>(pkt)), length, offset)
    {}

//    Pkt(Packet* pkt) :
//        SUBLAYER(move(std::unique_ptr<Packet, PacketDeleter>(pkt)))
//    {}

    uint8_t* data()
    {
        return getPtr().get()->data();
    }

    const uint8_t* data() const
    {
        return getPtr().get()->data();
    }

    uint16_t dataLength() const
    {
        return getPtr().get()->dataLength();
    }

    PacketDetails* getDetails()
    {
        return getPtr().get()->getDetails();
    }

    const PacketDetails* getDetails() const
    {
        return getPtr().get()->getDetails();
    }

    Packet::Proto proto() const
    {
        return getPtr().get()->proto;
    }

    Eth<Pkt> makeEth()
    {
        return Eth<Pkt>(move(*this));
    }

    Mtp<Pkt> makeMtp()
    {
        return Mtp<Pkt>(move(*this));
    }

    Mtp3<Pkt> makeMtp3()
    {
        return Mtp3<Pkt>(move(*this));
    }

    Hdlc<Pkt> makeHdlc()
    {
        return Hdlc<Pkt>(move(*this));
    }

    Gfp<Pkt> makeGfp()
    {
        return Gfp<Pkt>(move(*this));
    }

    Pkt clone()
    {
        ++getPtr().get()->refcnt;
        return Pkt(getLength(), getPtr().get());
    }

    void addPacket(Pkt&& packet)
    {
        if(getPtr().get())
            getPtr().get()->addPacket(packet.release());
        else
            setPtr(move(packet.getPtr()));
    }

    bool hasNext() const
    {
        return getPtr().get()->hasNext();
    }

    Packet * getNext() {
        return getPtr().get()->next;
    }
};

struct PktOffset : public PacketOffsetPtr<std::unique_ptr<Packet, PacketDeleter>, Packet>
{
    typedef PacketOffsetPtr<std::unique_ptr<Packet, PacketDeleter>, Packet> SUBLAYER;
    typedef PktOffset BaseType;

    PktOffset()
    {}

    PktOffset(SUBLAYER&& pkt) :
        SUBLAYER(move(pkt))
    {}

    PktOffset(Pkt&& pkt) :
        SUBLAYER(move(pkt))
    {}

    PktOffset& operator=(PktOffset&& pkt)
    {
        setPtr(move(pkt.getPtr()));
        setOffset(pkt.getOffset());
        return *this;
    }

    PktOffset& operator=(Pkt&& pkt)
    {
        setPtr(move(pkt.getPtr()));
        setOffset(pkt.getOffset());
        return *this;
    }

    PktOffset(typename SUBLAYER::BasePtr&& pkt, uint16_t length, uint16_t offset = sizeof(Packet)) :
        SUBLAYER(move(pkt), length, offset)
    {}

    PktOffset(uint16_t length, Packet* pkt, uint16_t offset = sizeof(Packet)) :
        SUBLAYER(move(std::unique_ptr<Packet, PacketDeleter>(pkt)), length, offset)
    {}

//    PktOffset(Packet* pkt) :
//        SUBLAYER(move(std::unique_ptr<Packet, PacketDeleter>(pkt)))
//    {}

    uint8_t* data()
    {
        return getPtr().get()->data();
    }

    const uint8_t* data() const
    {
        return getPtr().get()->data();
    }

    PacketDetails* getDetails()
    {
        return getPtr().get()->getDetails();
    }

    const PacketDetails* getDetails() const
    {
        return getPtr().get()->getDetails();
    }

    Packet::Proto proto() const
    {
        return getPtr().get()->proto;
    }

    PktOffset clone()
    {
        ++getPtr().get()->refcnt;
        return PktOffset(getLength(), getPtr().get(), getOffset());
    }
};


