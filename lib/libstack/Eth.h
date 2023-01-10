// Copyright: https://github.com/mikerez/mediaroom/blob/main/LICENSE

#pragma once

#include "HeaderIf.h"
#include "Vlan.h"
#include "Mpls.h"
#include "Ip4.h"

struct EthHdr
{
    uint8_t dest[6];
    uint8_t source[6];
    uint16_t type;
};
BUILD_ASSERT(sizeof(EthHdr) == 14);

template<class SUBLAYER>
struct Eth : public HeaderIf<EthHdr,Eth<SUBLAYER>,SUBLAYER>
{
    Ip4<Eth<SUBLAYER>> makeIp4() { return move(Ip4<Eth<SUBLAYER>>(move(*this))); }
    Vlan<Eth<SUBLAYER>> makeVlan() { return move(Vlan<Eth<SUBLAYER>>(move(*this))); }
    Mpls<Eth<SUBLAYER>> makeMpls() { return move(Mpls<Eth<SUBLAYER>>(move(*this))); }

    enum Type: uint16_t // inverse byte order
    {
        EthIP4   = 0x0008,
        EthVlan  = 0x0081,
        EthQinQ  = 0xA888,
        EthMplsU = 0x4788,  // unicast
        EthMplsM = 0x4888,  // multicast
    };

    typedef HeaderIf<EthHdr, Eth<SUBLAYER>, SUBLAYER> Sub;
    Eth(SUBLAYER&& pkt) : Sub(move(pkt)) {
    }
};


