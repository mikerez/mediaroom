// Copyright: https://github.com/mikerez/mediaroom/blob/main/LICENSE

#pragma once

#include "HeaderIf.h"
#include "Isup.h"
#include "Tcap.h"

struct SccpHdr
{
    uint8_t mt; // Message type
};

BUILD_ASSERT(sizeof(struct SccpHdr) == 1);

template<class SUBLAYER>
struct Sccp : public HeaderIf<SccpHdr, Sccp<SUBLAYER>, SUBLAYER>
{
    enum Type : uint8_t {
        T_CR    = 0x01, // Connection request
        T_CC    = 0x02, // Connection confirm
        T_CREF  = 0x03, // Connection refused
        T_RLSD  = 0x04, // Released
        T_RLC   = 0x05, // Release complete
        T_DT1   = 0x06, // Data form 1
        T_DT2   = 0x07, // Data form 2
        T_AK    = 0x08, // Data acknowledgement
        T_UDT   = 0x09, // Unitdata
        T_UDTS  = 0x0a, // Unitdata service
        T_ED    = 0x0b, // Expedited data
        T_EA    = 0x0c, // Expedited data acknowledgement
        T_RSR   = 0x0d, // Reset request
        T_RSC   = 0x0e, // Reset confirm
        T_ERR   = 0x0f, // Protocol data unit error
        T_IT    = 0x10, // Inactivity test
        T_XUDT  = 0x12, // Extended unitdata
        T_XUDTS = 0x13, // Extended unitdata service
        T_LUDT  = 0x14, // Long unitdata
        T_LUDTS = 0x15, // Long unitdata
    };

    enum Parameter : uint8_t
    {
        P_END     = 0x00, // End of optional parameters
        P_DST     = 0x01, // Destination local reference
        P_SRC     = 0x02, // Source local reference
        P_CALLED  = 0x03, // Called party address
        P_CALLING = 0x04, // Calling party address
        P_PC      = 0x05, // Protocol class
        P_SEGREAS = 0x06, // Segmenting / reassembling
        P_RCN     = 0x07, // Receive sequence number
        P_SEQSEG  = 0x08, // Sequencing / segmenting
        P_CREDIT  = 0x09, // Credit
        P_RELEASE = 0x0a, // Release cause
        P_RETURN  = 0x0b, // Return cause
        P_RESET   = 0x0c, // Reset cause
        P_ERR     = 0x0d, // Error cause
        P_REFUSE  = 0x0e, // Refusal cause
        P_DATA    = 0x0f, // Data
        P_SEG     = 0x10, // Segmentation
        P_HC      = 0x11, // Hop counter
        P_IMP     = 0x12, // Importance
        P_LDATA   = 0x13, // Long data
    };

    enum SubsystemNumber : uint8_t
    {
        SSN_UNKNOWN = 0x0,
        SSN_SCMG    = 0x1, // SCCP management
        SSN_ISUP    = 0x3, // ISDN user part
        SSN_OMAP    = 0x4, // operation, maintenance and administration part
        SSN_MAP     = 0x5, // mobile application part
        SSN_HLR     = 0x6, // home location register
        SSN_VLR     = 0x7, // visitor location register
        SSN_MSC     = 0x8, // mobile switching centre
        SSN_EIC     = 0x9, // equipment identifier centre
        SSN_AUC     = 0xa, // authentication centre
        SSN_ISDNSS  = 0xb, // ISDN supplementary services
        SSN_INAP    = 0xc, // Intelligent Network Application Protocol
        SSN_BISDN   = 0xd, // broadband ISDN edge - to - edge applications
        SSN_TCTR    = 0xe, // TC test responder

        /* The following national network subsystem numbers have been allocated
         * for use within and between GSM/UMTS networks: */

        SSN_RANAP   = 0x8e, // Radio Access Network Application Part
        SSN_RNSAP   = 0x8f, // Radio Network System Application Part
        SSN_GMLC    = 0x91, // Gateway Mobile Location Centre
        SSN_CAP     = 0x92, // CAMEL Application Part
        SSN_GSMSCF  = 0x93, // gsmSCF (MAP) or IM-SSF (MAP) or Presence Network Agent
        SSN_SIWF    = 0x94,
        SSN_SGSN    = 0x95, // Serving GPRS support node
        SSN_GGSN    = 0x96, // Gateway GPRS support node

        /* The following national network subsystem numbers have been allocated
         * for use within GSM/UMTS networks: */

        SSN_MTX        = 0xf8,
        SSN_HLRMUP     = 0xf9,
        SSN_BSCBSSAPP  = 0xfa, // Base station controller (BSSAP-LE)
        SSN_MSCBSSAP   = 0xfb, // Mobile switching center (BSSAP-LE)
        SSN_SMLCBSSAPP = 0xfc, // Serving Mobile Location Center (SMLC) (BSSAP-LE)
        SSN_BSSOM      = 0xfd, // Base station subsystem O&M (A interface)
        SSN_BSSAP      = 0xfe, // Base Station Subsystem Application Part (BSSAP/BSAP)
    };

    uint8_t* getWithOffset(uint16_t offset)
    {
        return (uint8_t*)Sub::layer()->get() + Sub::layer()->getOffset() + offset;
    }

    Tcap<Sccp<SUBLAYER>> makeTcap() { return move(Tcap<Sccp<SUBLAYER>>(move(*this))); }

    typedef HeaderIf<SccpHdr, Sccp<SUBLAYER>, SUBLAYER> Sub;
    Sccp(SUBLAYER&& pkt) : Sub(move(pkt)) {}
};
