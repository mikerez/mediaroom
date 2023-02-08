// Copyright: https://github.com/mikerez/mediaroom/blob/main/LICENSE

/*
 * Signalling System No. 7 – Message transfer part 3 implementation of
 *
 * ITU-T Q.703 (07/96)
 * ITU-T Q.704 (07/96)
 *
 * SERIES Q: SWITCHING AND SIGNALLING
 * Specifications of Signalling System No. 7 – Message transfer part
 *
 * ----------------------------------------------------------------------------
 *
 */

#pragma once

#include "HeaderIf.h"
#include "Sccp.h"
#include "ss7types.h"

/*
 * ITU-T Q.703
 * 2.2 Signal unit format
 * Three types of signal unit are differentiated by means of the length
 * indicator contained in all signal units, i.e. message signal units, link
 * status signal units and fill-in signal units. Message signal units are
 * retransmitted in case of error, link status signal unit and fill-in signal
 * units are not. The basic formats of the signal units are shown in Figure 3.
 *
 * ---------------------------------------------------------------
 * | F | CK | SIF     | SIO | - | LI | FIB | FSN | BIB | BSN | F |
 * ---------------------------------------------------------------
 * | 8 | 16 | 8n, n≥2 |  8  | 2 |  6 |  1  |  7  |  1  |  7  | 8 | First bit transmitted
 * ---------------------------------------------------------------
 *  a) Basic format of a Message Signal Unit (MSU)
 *
*/

#pragma pack(push, 1)
struct Mtp3Hdr
{
    sio_t sio;
    routing_label_t rl;
};
#pragma pack(pop)
BUILD_ASSERT(sizeof(Mtp3Hdr) == 5);

template<class SUBLAYER>
struct Mtp3 : public HeaderIf<Mtp3Hdr, Mtp3<SUBLAYER>, SUBLAYER>
{
    Isup<Mtp3<SUBLAYER>> makeIsup() { return move(Isup<Mtp3<SUBLAYER>>(move(*this))); }
    Sccp<Mtp3<SUBLAYER>> makeSccp() { return move(Sccp<Mtp3<SUBLAYER>>(move(*this))); }

    // Network Indicator 
    enum NI : uint8_t {
        INT, // International
        // 1 International Spare
        NAT = 2, // National
        // 3 National Spare
    };

    // Service Indicator
    enum SI : uint8_t {
        // 0 Signaling Network Management Messages
        // 1 Signaling Network Testing and Maintenance Messages
        // 2 Signaling Network Testing and Maintenance Special Messages (ANSI) or Spare (ITU-T)
        SCCP = 3,
        TUP, // Telephone User Part
        ISUP, // ISDN User Part
        DUPCC, // Data User Part (call and circuit-related messages)
        DUPFRC, // Data User Part (facility registration and cancellation messages)
        MTPTUP, // Reserved for MTP Testing User Part
        BISUP, // Broadband ISDN User Part
        SISUP // Satellite ISDN User Part
        // Spare 11-15
    };

    typedef HeaderIf<Mtp3Hdr, Mtp3<SUBLAYER>, SUBLAYER> Sub;
    Mtp3(SUBLAYER&& pkt) : Sub(move(pkt)) {
    }
};