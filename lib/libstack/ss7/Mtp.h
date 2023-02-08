// Copyright: https://github.com/mikerez/mediaroom/blob/main/LICENSE

/*
 * Signalling System No. 7 – Message transfer part implementation of 
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
#include "Isup.h"
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
 *       ---------------------------------------------------------
 *       | F | CK | SF      | - | LI | FIB | FSN | BIB | BSN | F |
 *       ---------------------------------------------------------
 *       | 8 | 16 | 8 or 16 | 2 | 6  |  1  |  7  |  1  |  7  | 8 | First bit transmitted
 *       ---------------------------------------------------------
 *  b) Basic format of a Link Status Signal Unit (LSSU)
 *
 *                 -----------------------------------------------
 *                 | F | CK | - | LI | FIB | FSN | BIB | BSN | F |
 *                 -----------------------------------------------
 *                 | 8 | 16 | 2 | 6  |  1  |  7  |  1  |  7  | 8 | First bit transmitted
 *                 -----------------------------------------------
 *  c) Basic format of a Fill-In Signal Unit (FISU)
 *
 * BIB - Backward Indicator Bit
 * BSN - Backward Sequence Number
 * CK  - Check bits
 * F   - Flag
 * FIB - Forward Indicator Bit
 * FSN - Forward Sequence Number
 * LI  - Length Indicator
 * n   - Number of octets in the SIF
 * SF  - Status Field
 * SIF - Signalling Information Field
 * SIO - Service Information Octet
 *                Figure 3/Q.703 – Signal unit formats
*/
typedef struct mtp_bsn
{
    // 2.3.5 Sequence numbering
    /*
     * The backward sequence number is the sequence number of a signal unit being
     * acknowledged.
     */
    uint8_t bsn : 7;
    /*
     * 2.3.6 Indicator bits
     *
     * The forward indicator bit and backward indicator bit together with the
     * forward sequence number and backward sequence number are used in the basic
     * error control method to perform the signal unit sequence control and
     * acknowledgement functions (see 5.2 and clause 6).
     */
    uint8_t bib : 1;
} mtp_bsn_t;

typedef struct mtp_fsn
{
    /*
     * The forward sequence number is the sequence number of the signal unit in
     * which it is carried.
     */
    uint8_t fsn : 7;
    uint8_t fib : 1;
} mtp_fsn_t;

struct MtpHdr
{
    // TODO: starting and ending F,CK fields are shrinked ?
    mtp_bsn_t bsn;
    mtp_fsn_t fsn;

/*
 * 2.3.3 Length indicator
 *
 * The length indicator is used to indicate the number of octets following the
 * length indicator octet and preceding the check bits and is a number in binary
 * code in the range 0-63. The length indicator differentiates between the three
 * types of signal units as follows:
 * - Length indicator = 0: Fill-in signal unit
 * - Length indicator = 1 or 2: Link status signal unit
 * - Length indicator > 2: Message signal unit
 *
 * In the case that the signalling information field of a message signal unit is
 * spanning 62 octets or more, the length indicator is set to 63.
 * It is mandatory that LI is set by the transmitting end to its correct value
 * as specified above
 */
    li_t li;
/*
 * 11 Level 2 codes and priorities
 * 11.1 Link status signal unit
 *
 * 11.1.1 The link status signal unit is identified by a length indicator value
 * equal to 1 or 2. If the length indicator has a value of 1 then the status
 * field consists of one octet; if the length indicator has a value of 2 then
 * the status field consists of two octets.
 *
 * 11.1.2 The format of the one octet status field is shown in Figure 6.
 * When a terminal, which is able to process only a one-octet status field,
 * receives a link status signal unit with a two-octet status field, the
 * terminal shall ignore the second octet for compatibility reasons but process
 * the first octet as specified.
 *                   -----------------------
 *                   | Spare | Status      |
 *                   |       | indications |
 *                   |       |-------------|
 *                   |       |  C | B | A  |
 *                   -----------------------
 *                   |   5   |      3      | First bit transmitted
 *                   -----------------------
 *                   Figure 6/Q.703 – Status field format
 *
 * 11.1.3 The use of the link status indications is described in clause 7.
 * They are coded as follows:
 * C B A
 * 0 0 0 – Status indication "O"
 * 0 0 1 – Status indication "N"
 * 0 1 0 – Status indication "E"
 * 0 1 1 – Status indication "OS"
 * 1 0 0 – Status indication "PO"
 * 1 0 1 – Status indication "B"
 * The spare bits should be ignored at the receiving side.
 */
    sio_t sio;
    routing_label_t rl;
 // TODO: ANNEX A Additions for a national option for high speed signalling links
};

BUILD_ASSERT(sizeof(MtpHdr) == 8);

template<class SUBLAYER>
struct Mtp : public HeaderIf<MtpHdr, Mtp<SUBLAYER>,SUBLAYER>
{
    Isup<Mtp<SUBLAYER>> makeIsup() { return move(Isup<Mtp<SUBLAYER>>(move(*this))); }
    Sccp<Mtp<SUBLAYER>> makeSccp() { return move(Sccp<Mtp<SUBLAYER>>(move(*this))); }

    // Network Indicator 
    enum NI: uint8_t {
            INT, // International
            // 1 International Spare
            NAT = 2, // National
            // 3 National Spare
    };

    // Service Indicator
    enum SI: uint8_t {
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

    // Link Status Indication
    enum LSI: uint8_t
    {
            SIO = 0, // O: Out of Alignment: Link not aligned; attempting alignment
            SIN, // N: Normal Alignment: Link is aligned
            SIE, // E: Emergency Alignment: Link is aligned
            SIOS, // OS: Out of Service: Link out of service; alignment failure
            SIPO, // PO: Processor Outage: MTP2 cannot reach MTP3
            SIB // B: Busy: MTP2 congestion
    };

    enum Type: uint8_t
    {
        FISU = 0, // Fill-In Signal Unit
        LSSU = 1, // Link Status Signal Unit
        MSU  = 3, // Message Signal Unit
        MSU_MAX = 63, // Message Signal Unit
    };

    typedef HeaderIf<MtpHdr, Mtp<SUBLAYER>, SUBLAYER> Sub;
    Mtp(SUBLAYER&& pkt) : Sub(move(pkt)) {
    }
};
