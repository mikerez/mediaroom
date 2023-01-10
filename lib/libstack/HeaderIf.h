// Copyright: https://github.com/mikerez/mediaroom/blob/main/LICENSE

#pragma once

#include "LibStack.h"

template<class HDR, class LAYER, class SUBLAYER>
struct HeaderIf: public SUBLAYER
{
    typedef HDR Hdr;

    LAYER* layer()
    {
        return (LAYER*)this;
    }

    const LAYER* layer() const
    {
        return (const LAYER*)this;
    }

    SUBLAYER* sublayer()
    {
        return static_cast<SUBLAYER*>(layer());
    }

    const SUBLAYER* sublayer() const
    {
        return static_cast<const SUBLAYER*>(layer());
    }

    HDR* hdr()
    {
        return (HDR*)((uint8_t*)layer()->get() + layer()->getOffset() + sublayer()->fullSize());
    }

    HDR* operator->()
    {
        return (HDR*)((uint8_t*)layer()->get() + layer()->getOffset() + sublayer()->fullSize());
    }

    //typename SUBLAYER::Hdr* prev()
    //{
    //    ??? Wrong result !!!
    //    return (typename SUBLAYER::Hdr*)((uint8_t*)layer()->get() + layer()->getOffset() + sublayer()->fullSize());
    //}

    typename SUBLAYER::Hdr* sublayerHdr()
    {
        return sublayer()->hdr();
    }

    typename SUBLAYER::BaseType push()
    {
        return move(typename SUBLAYER::BaseType(move(layer()->getPtr()), layer()->getLength() - layer()->fullSize(), layer()->getOffset() + layer()->fullSize()));
    }
    typename SUBLAYER::BaseType rebase()
    {
        return move(typename SUBLAYER::BaseType(move(layer()->getPtr()), layer()->getLength() - sublayer()->fullSize(), layer()->getOffset() + sublayer()->fullSize()));
    }

    uint16_t fullSize()
    {
        return layer()->size() + sublayer()->fullSize();
    }

    const uint16_t fullSize() const
    {
        return layer()->size() + sublayer()->fullSize();
    }

    uint16_t size() const
    {
        return sizeof(HDR);
    }

    uint8_t* payload()
    {
        LS_ASSERT(layer()->getLength() >= layer()->size());  // packet is shorter than markup
        return (uint8_t*)layer()->get() + layer()->getOffset() + layer()->fullSize();
    }

    const uint8_t* payload() const
    {
        LS_ASSERT(layer()->getLength() >= layer()->size());  // packet is shorter than markup
        return (const uint8_t*)layer()->get() + layer()->getOffset() + layer()->fullSize();
    }

    uint16_t payloadLength()
    {
        LS_ASSERT(layer()->getLength() >= layer()->size());  // packet is shorter than markup
        return layer()->getLength() - layer()->fullSize();
    }

#pragma pack(1)
    struct Format: public SUBLAYER::Format
    {
        HDR hdr;
    };
#pragma pack()

    HeaderIf(SUBLAYER&& pkt) : SUBLAYER(move(pkt)) {
    }
    HeaderIf() : SUBLAYER() {
    }
};

