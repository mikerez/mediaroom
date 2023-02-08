// Copyright: https://github.com/mikerez/mediaroom/blob/main/LICENSE

#pragma once

#include "HeaderIf.h"

struct TcapHdr
{
    uint8_t tmp;
};

BUILD_ASSERT(sizeof(struct TcapHdr) == 1);

template<class SUBLAYER>
struct Tcap : public HeaderIf<TcapHdr, Tcap<SUBLAYER>, SUBLAYER>
{
    uint8_t* payload()
    {
        return (uint8_t*)Sub::layer()->get() + Sub::layer()->getOffset() + Sub::sublayer()->fullSize();
    }

    uint16_t payloadLength()
    {
        return Sub::layer()->getLength() - Sub::sublayer()->fullSize();
    }

    typedef HeaderIf<TcapHdr, Tcap<SUBLAYER>, SUBLAYER> Sub;
    Tcap(SUBLAYER&& pkt) : Sub(move(pkt)) {}
};
