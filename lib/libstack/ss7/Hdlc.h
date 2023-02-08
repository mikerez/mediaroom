// Copyright: https://github.com/mikerez/mediaroom/blob/main/LICENSE

/*
 * High-Level Data Link Control - ISO/IEC 13239:2002(E)
 *
 */
#pragma once

#include "HeaderIf.h"
#include "Isup.h"

/*
 * 4.1.1 Basic frame format
 * Each frame using the basic frame format consists of the following fields 
 * (transmission sequence left to right):
 *
 *  Flag     | Address | Control | Info. | FCS     | Flag
 *  ---------------------------------------------------------
 *  01111110 | 8 bits  | 8 bits  | *     | 16 bits | 01111110
 *  
 *  An unspecified number of bits which in some cases may be a multiple of a 
 *  particular character size; for example, an octet.
 *  where
 *      Flag = flag sequence
 *      Address = data station address field
 *      Control = control field
 *      Information = information field
 *      FCS = frame checking sequence field
 *
 *  Frames containing only control sequences form a special case where there is
 *  no information field.
 *
 *  4.1.2 Non-basic frame format 
 *  A frame using the non-basic frame format does not follow the structure of 
 *  4.1.1 in one or more ways. For example, a frame using a non-basic frame format 
 *  - instead of having only one address field, has more than one address field (see 4.2.2); or
 *  - instead of an address field consisting of a single octet, has an extended
 *    address field consisting of one or more octets (see 4.7.1 and 6.15.7); or
 *  - instead of a basic control field consisting of a single octet, has an 
 *    extended control field of more than one octet (see 4.7.2 and 6.15.10); or
 *  - instead of a 16-bit FCS, has an 8-bit FCS (see 4.2.5.4 and 6.15.14.2) or
 *    a 32-bit FCS (see 4.2.5.3 and 6.15.14.1); or
 *  - instead of being transmitted in synchronous mode, is sent in start/stop
 *    mode (see 4 and 6.15.15); or
 *  - instead of having an address field follow the opening flag sequence, has
 *    a frame format field following the opening flag sequence; or 
 *  - instead of having the information field follow the control field, there
 *    may be a header check sequence following the control field.
 */
struct HdlcHdr
{
    uint8_t address;
    /*
     * 5.3 Control field formats
     * 5.3.1 General
     * The three formats defined for the basic (modulo 8) control field (see 
     * table 3) are used to perform numbered information transfer, numbered 
     * supervisory functions and unnumbered control functions and unnumbered
     * information transfera
     *
     * Table 3 Control field formats for modulo 8
     * ------------------------------------------------------------------------
     * Control field format for              |      Control field bits         |
     *                                       |---------------------------------
     *                                       | 1 | 2 | 3 | 4 |  5  | 6 | 7 | 8 |
     * ------------------------------------------------------------------------
     * Information transfer command/response | 0 |  N(S)     | P/F |   N(R)    |
     * (I format)                            |   |           |     |           |
     * ------------------------------------------------------------------------
     * Supervisory commands/responses        | 1 | 0 | S | S | P/F |   N(R)    |
     * (S format)                            |   |   |   |   |     |           |
     * ------------------------------------------------------------------------
     * Unnumbered commands/responses         | 1 | 1 | M | M | P/F | M | M | M |
     * (U format)                            |   |   |   |   |     |   |   |   |
     * ------------------------------------------------------------------------
     * N(S) = transmitting send sequence number (bit 2 = low-order bit)
     * N(R) = transmitting receive sequence number (bit 6 = low-order bit)
     * S = supervisory function bit
     * M = modifier function bit
     * P/F = poll bit - primary station or combined station command frame
     *                  transmissions/final bit 
     *                - secondary station or combined station response
     *                  frame transmissions (1 = poll/final).
     */
    uint8_t control;
};

BUILD_ASSERT(sizeof(HdlcHdr) == 2);

template<class SUBLAYER>
struct Hdlc : public HeaderIf<HdlcHdr,Hdlc<SUBLAYER>,SUBLAYER>
{
    Isup<Hdlc<SUBLAYER>> makeIsup() { return move(Isup<Hdlc<SUBLAYER>>(move(*this))); }

    typedef HeaderIf<HdlcHdr, Hdlc<SUBLAYER>, SUBLAYER> Sub;
    Hdlc(SUBLAYER&& pkt) : Sub(move(pkt)) {
    }
};
