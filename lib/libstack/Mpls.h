// Copyright: https://github.com/mikerez/mediaroom/blob/main/LICENSE

#pragma once

#include "HeaderIf.h"
#include "Ip4.h"

template<class SUBLAYER>
struct Eth;

struct MplsHdr {
    uint32_t    label_0 : 16;
    uint32_t    bs      : 1;
    uint32_t    tc      : 3;
    uint32_t    label_1 : 4;
    uint32_t    ttl     : 8;

    uint32_t getLabel() const   { return label_0 + (label_1 << 16); }
    uint8_t  getTc()    const   { return tc; }
    uint8_t  getBs()    const   { return bs; }
    uint8_t  getTtl()   const   { return ttl; }
};
BUILD_ASSERT(sizeof(MplsHdr) == 4);

template<class SUBLAYER>
struct Mpls : public HeaderIf<MplsHdr,Mpls<SUBLAYER>,SUBLAYER>
{
    Ip4<Mpls<SUBLAYER>> makeIp4() { return move(Ip4<Mpls<SUBLAYER>>(move(*this))); }
    Mpls<Mpls<SUBLAYER>> makeMpls() { return move(Mpls<Mpls<SUBLAYER>>(move(*this))); }
    Eth<Mpls<SUBLAYER>> makeEth() { return move(Eth<Mpls<SUBLAYER>>(move(*this))); }

    enum Type: uint8_t
    {
        MplsIP4 = 1,
        MplsMpls = 0
    };

    typedef HeaderIf<MplsHdr, Mpls<SUBLAYER>, SUBLAYER> Sub;
    Mpls(SUBLAYER&& pkt) : Sub(move(pkt)) {}
};


