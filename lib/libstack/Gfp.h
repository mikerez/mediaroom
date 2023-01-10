// Copyright: https://github.com/mikerez/mediaroom/blob/main/LICENSE

#pragma once

// ITU-T Rec. G.7041/Y.1303 (08/2005) Generic framing procedure (GFP)

#include "HeaderIf.h"

/*
 * Two kinds of GFP frames are defined: GFP client frames and GFP control frames
 *
 * 6.1 Basic signal structure for GFP client frames
 * GFP frames are octet-aligned and consist of a
 * GFP Core Header and, except for GFP Idle frames, a GFP Payload Area.
 *
 * ------------------------------------------------------------------------------------------------------
 *                                      |                           Payload Area
 *             Core Header              |----------------------------------------------------------------
 *                                      |          Payload Header          |   Payload   | Payload Frame
 * ------------------------------------------------------------------------| Information | Check Sequence
 *  Payload Length  | Core Header Error |  Type  | Type HEC |   Extension  |    Field    |  (pFCS) field
 *  Indicator (PLI) |    Check (cHEC)   |        |  (tHEC)  | Header Field |    (PIF)    |
 * ------------------------------------------------------------------------------------------------------
 *      16 bit      |       16 bit      | 16 bit |  16 bit  |   Optional   |
 * ------------------------------------------------------------------------------------------------------
 */

#pragma pack(push, 1)
 struct GfpHdr {
    /* The two-octet PLI field contains a binary number representing the number of octets
     * in the GFP Payload Area
     */
    uint16_t pli;
    /* The two-octet Core Header Error Control field contains a CRC-16 error control code that protects
     * the integrity of the contents of the Core Header by enabling both single-bit error correction and
     * multi-bit error detection.
     */
    uint16_t cHec;
};

/*
 * - The GFP Type field is a mandatory two-octet field of the Payload Header that
 * indicates the content and format of the GFP Payload Information field:
 *
 * ------------------------------------------------------
 *    Payload Type   |   Payload FCS   | Extension Header
 *  Identifier (PTI) | Indicator (PFI) | Identifier (EXI)
 * ------------------------------------------------------
 *             User Payload Identifier (UPI)
 * ------------------------------------------------------
 */

struct GdpType {
    /*
     * Payload type identifier  type of GFP client frame.
     * GFP payload type identifiers
     * -----------------------------
     * Type bits | Usage
     * -----------------------------
     *    000    | Client Data
     *    100    | Client Management 
    */
    uint8_t pti : 3;
    /*
     * 6.1.2.1.1.2 Payload FCS Indicator (PFI) 
     * indicating the presence (PFI = 1) or absence (PFI = 0) of the Payload FCS field
     */
    uint8_t pfi : 1;
    /* GFP extension header identifiers:
     * ---------------------------------
     * Type bits | Usage
     * ---------------------------------
     *    0000   | Null Extension Header
     *    0001   | Linear Frame
     *    0010   | Ring Frame
     */
    uint8_t exi : 4;
    uint8_t upi;
};

struct GfpPayloadHdr {
    GdpType type;
    /*
     * The two-octet Type Header Error Control field contains a CRC-16 error control code
     * that protects the integrity of the contents of the Type Field by enabling both
     * single-bit error correction and multibit error detection.
    */
    uint16_t tHec;
};

struct LinearframeHdr {
    uint8_t cid;
    uint8_t spare;
    uint16_t eHec;
};
#pragma pack(pop)
BUILD_ASSERT(sizeof(GfpHdr) == 4);
BUILD_ASSERT(sizeof(GfpPayloadHdr) == 4);

/* User payload identifiers for GFP client frames */
enum class UPI {
    F_ETH = 0x01, // Frame - Mapped Ethernet
    F_PPP = 0x02, // Frame - Mapped PPP
    T_FIBRE_CHANNEL = 0x3, // Transparent Fibre Channel
    T_FICON = 0x04, // Transparent FICON
    T_ESCON = 0x05, // Transparent ESCON
    T_GB_ETH = 0x06, // Transparent Gb Ethernet
    F_MAPOS = 0x08, // Frame - Mapped Multiple Access Protocol over SDH(MAPOS)
    T_DVB_ASI = 0x09, // Transparent DVB ASI
    F_IEEE_802_17 = 0x0A, // Framed - Mapped IEEE 802.17 Resilient Packet Ring
    F_BBW = 0x0B, // Frame - Mapped Fibre Channel FC - BBW
    AT_FIBRE_CHANNEL = 0x0C, // Asynchronous Transparent Fibre Channel
    F_MPLS_UNI = 0x0D, // Frame - Mapped MPLS(Unicast)
    F_MPLS_MULTI = 0x0E, // Frame - Mapped MPLS(Multicast)
    F_ISIS = 0x0F, // Frame - Mapped IS - IS
    F_IPV4 = 0x10, // Frame - Mapped IPv4
    F_IPV6 = 0x11, // Frame - Mapped IPv6
    F_DVB_ASI = 0x12, // Frame - mapped DVB - ASI
};

/*
 * 6.2 GFP control frames
 * GFP control frames are used in the management of the GFP connection.The only control frame
 * specified at this time is the GFP Idle frame.
 */
#define GFP_CONTROL_FRAME 0xB6AB31E0


template<class SUBLAYER>
struct Gfp : public HeaderIf<GfpHdr, Gfp<SUBLAYER>, SUBLAYER>
{
    typedef HeaderIf<GfpHdr, Gfp<SUBLAYER>, SUBLAYER> Sub;
    Gfp(SUBLAYER&& pkt) : Sub(move(pkt)) {
    }
};
