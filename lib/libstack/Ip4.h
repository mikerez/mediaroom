// Copyright: https://github.com/mikerez/mediaroom/blob/main/LICENSE

#pragma once

#include "HeaderIf.h"
#include "Tcp.h"
#include "Udp.h"
#include "ss7/Sctp.h"
#include <stdio.h>

struct Ip4Hdr
{
    uint8_t header_len : 4;
    uint8_t version : 4;
    uint8_t tos;
    uint16_t total_length = 0;
    uint16_t id;
    uint8_t frag_offset : 5;
    uint8_t more_fragment : 1;
    uint8_t dont_fragment : 1;
    uint8_t reserved_zero : 1;
    uint8_t frag_offset1;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t checksum;
    uint32_t srcaddr;
    uint32_t dstaddr;
};
BUILD_ASSERT(sizeof(Ip4Hdr) == 20);

template<class SUBLAYER>
struct Ip4 : public HeaderIf<Ip4Hdr,Ip4<SUBLAYER>,SUBLAYER>
{
    Ip4<Ip4<SUBLAYER>> makeIp4() { return move(Ip4<Ip4<SUBLAYER>>(move(*this))); }
    Tcp<Ip4<SUBLAYER>> makeTcp() { return move(Tcp<Ip4<SUBLAYER>>(move(*this))); }
    Udp<Ip4<SUBLAYER>> makeUdp() { return move(Udp<Ip4<SUBLAYER>>(move(*this))); }
    Sctp<Ip4<SUBLAYER>> makeSctp() { return move(Sctp<Ip4<SUBLAYER>>(move(*this))); }

    enum Type : uint8_t {
        Ip4Tcp = 0x06,
        Ip4Udp = 0x11,
        Ip4Sctp = 0x84,
        Ip4Ip4 // todo
    };

    uint16_t size() {
        return Sub::hdr()->header_len << 2;
    }

    uint16_t getLength() {
        return LS_ntohs(Sub::hdr()->total_length);
    }

    typedef HeaderIf<Ip4Hdr, Ip4<SUBLAYER>, SUBLAYER> Sub;
    Ip4(SUBLAYER&& pkt) : Sub(move(pkt)) {}
};
