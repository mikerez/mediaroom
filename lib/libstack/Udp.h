// Copyright: https://github.com/mikerez/mediaroom/blob/main/LICENSE

#pragma once

#include "HeaderIf.h"

struct UdpHdr
{
    uint16_t source_port;
    uint16_t dest_port;
    uint16_t udp_length;
    uint16_t udp_checksum;
};
BUILD_ASSERT(sizeof(struct UdpHdr) == 8);

template<class SUBLAYER>
struct Udp : public HeaderIf<UdpHdr,Udp<SUBLAYER>,SUBLAYER>
{
    enum Type
    {
    };

    typedef HeaderIf<UdpHdr, Udp<SUBLAYER>, SUBLAYER> Sub;
    Udp(SUBLAYER&& pkt) : Sub(move(pkt)) {}
};
