// Copyright: https://github.com/mikerez/mediaroom/blob/main/LICENSE

#pragma once

#include "HeaderIf.h"
#include "Mpls.h"
#include "Ip4.h"

struct VlanHdr {
    uint16_t    tci;
    uint16_t    tpid;
};
BUILD_ASSERT(sizeof(VlanHdr) == 4);

template<class SUBLAYER>
struct Vlan : public HeaderIf<VlanHdr,Vlan<SUBLAYER>,SUBLAYER>
{
    Ip4<Vlan<SUBLAYER>> makeIp4() { return move(Ip4<Vlan<SUBLAYER>>(move(*this))); }
    Vlan<Vlan<SUBLAYER>> makeVlan() { return move(Vlan<Vlan<SUBLAYER>>(move(*this))); }
    Mpls<Vlan<SUBLAYER>> makeMpls() { return move(Mpls<Vlan<SUBLAYER>>(move(*this))); }

    enum Type: uint16_t // inverse byte order
    {
        VlanIP4   = 0x0008,
        VlanVlan  = 0x0081,
        VlanMplsU = 0x4788, // unicast
        VlanMplsM = 0x4888, // multicast
    };

    typedef HeaderIf<VlanHdr, Vlan<SUBLAYER>, SUBLAYER> Sub;
    Vlan(SUBLAYER&& pkt) : Sub(move(pkt)) {}
};

