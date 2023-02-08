// Copyright: https://github.com/mikerez/mediaroom/blob/main/LICENSE

#pragma once

#include "HeaderIf.h"
#include "Mtp3.h"

struct M2uaParameter
{
    uint16_t tag;
    uint16_t length;
};

struct M2uaHdr
{
    uint8_t version;
    uint8_t reserved;
    uint8_t msgClass;
    uint8_t type;
    uint32_t length;
    M2uaParameter parameter;
};
BUILD_ASSERT(sizeof(M2uaHdr) == 12);

template<class SUBLAYER>
struct M2ua : public HeaderIf<M2uaHdr, M2ua<SUBLAYER>, SUBLAYER>
{
    enum MsgClass : uint8_t
    {
        MGMT  = 0x0,  // Management Message [IUA / M2UA / M3UA / SUA]
        TM    = 0x1,  // Transfer Messages [M3UA]
        SSNM  = 0x2,  // SS7 Signalling Network Management Messages [M3UA / SUA]
        ASPSM = 0x3,  // ASP State Maintenance Messages [IUA / M2UA / M3UA / SUA]
        ASPTM = 0x4,  // ASP Traffic Maintenance Messages [IUA / M2UA / M3UA / SUA]
        QPTM  = 0x5,  // Q.921 / Q.931 Boundary Primitives Transport Messages [IUA]
        MAUP  = 0x6,  // MTP2 User Adaptation Messages [M2UA]
        CM    = 0x7,  // Connectionless Messages [SUA]
        COM   = 0x8,  // Connection - Oriented Messages [SUA]
        RKM   = 0x9,  // Routing Key Management Messages (M3UA)
        IIM   = 0x10, // Interface Identifier Management Messages (M2UA)
    };

    Mtp3<M2ua<SUBLAYER>> makeMtp3() { return move(Mtp3<M2ua<SUBLAYER>>(move(*this))); }

    //uint16_t size()
    //{
    //    return LS_ntohl(Sub::hdr()->length);
    //}

    typedef HeaderIf<M2uaHdr, M2ua<SUBLAYER>, SUBLAYER> Sub;
    M2ua(SUBLAYER&& pkt) : Sub(move(pkt)) {}
};
