// Copyright: https://github.com/mikerez/mediaroom/blob/main/LICENSE

#pragma once

#include "HeaderIf.h"

struct M3uaHdr
{
    uint8_t version;
    uint8_t reserved;
    uint8_t msgClass;
    uint8_t type;
    uint32_t length;
};
BUILD_ASSERT(sizeof(M3uaHdr) == 8);

struct M3uaParamter
{
private:
    uint16_t tag;
    uint16_t length;

public:
    inline uint16_t getTag() const { return LS_ntohs(tag); }
    inline uint16_t getLength() const { return LS_ntohs(length); }
};

struct M3uaProtocolData
{
    enum SI : uint8_t
    {
        SCCP  = 0x3,
        TUP   = 0x4,  // Telephone User Part
        ISUP  = 0x5,  // ISDN User Part
        BISUP = 0x9,  // Broadband ISUP
        SISUP = 0x10, // Satellite ISUP
        AAL2  = 0x12, // AAL type 2 Signalling
        BICC  = 0x13, // Bearer Independent Call Control
        GCP   = 0x14, // Gateway Control Protocol
    };

    M3uaParamter param;
    uint32_t opc;	// Originating Point Code
    uint32_t dpk;	// Destination Point Code
    uint8_t  si;	// Service Indicator
    uint8_t  ni;	// Network Indicator
    uint8_t  mp;	// Message Priority
    uint8_t  sls;	// Signalling Link Selection Code
};

template<class SUBLAYER>
struct M3ua : public HeaderIf<M3uaHdr, M3ua<SUBLAYER>, SUBLAYER>
{
    enum MsgClass : uint8_t
    {
        MGMT  = 0x0, // Management Messages
        TM    = 0x1, // Transfer Messages
        SNMP  = 0x2, // SS7 Signalling Network Management Messages
        ASPSM = 0x3, // ASP State Maintenance Messages
        ASPTM = 0x4, // ASP Traffic Maintenance Messages
        RKM   = 0x8, // Routing Key Management Messages
    };

    enum TypeMask : uint16_t
    {
        MGMT_ERR      = 0x0000, // Error
        MGMT_NTFY     = 0x0001, // Notify

        TM_DATA       = 0x0101, // Data

        SNMP_DUNA     = 0x0201, // Destination Unavailable
        SNMP_DAVA     = 0x0202, // Destination Available
        SNMP_DAUD     = 0x0203, // Destination State Audit
        SNMP_SCON     = 0x0204, // Signalling Congestion
        SNMP_DUPU     = 0x0205, // Destination User Part Unavailable
        SNMP_DRST     = 0x0206, // Destination Restricted

        ASPSM_UP      = 0x0301, // ASP Up
        ASPSM_DN      = 0x0302, // ASP Down
        ASPSM_BEAT    = 0x0303, // Heartbeat
        ASPSM_UPACK   = 0x0304, // ASP Up Acknowledgement
        ASPSM_DNACK   = 0x0305, // ASP Down Acknowledgement
        ASPSM_BEATACK = 0x0306, // Heartbeat Acknowledgement

        ASPTM_AC      = 0x0401, // ASP Active(ASPAC)
        ASPTM_IA      = 0x0402, // ASP Inactive(ASPIA)
        ASPTM_ACACK   = 0x0403, // ASP Active Acknowledgement(ASPAC ACK)
        ASPTM_IAACK   = 0x0404, // ASP Inactive Acknowledgement(ASPIA ACK)

        RKM_REGREQ   = 0x0901, //  Registration Request(REG REQ)
        RKM_REGRSP   = 0x0902, //  Registration Response(REG RSP)
        RKM_DEREGREQ = 0x0903, //  Deregistration Request(DEREG REQ)
        RKM_DEREGRSP = 0x0904, //  Deregistration Response(DEREG RSP)
    };

    enum ParameterTag : uint16_t
    {
        // Common parameters
        INFO    = 0x0004, // INFO String
        RCTX    = 0x0006, // Routing Context
        DINFO   = 0x0007, // Diagnostic Information
        HBDATA  = 0x0009, // Heartbeat Data
        TMT     = 0x000b, // Traffic Mode Type
        ERR     = 0x000c, // Error Code
        STATUS  = 0x000d, // Status
        // M3UA-Specific parameters
        NA      = 0x0200, // Network Appearance
        USR     = 0x0204, // User / Cause
        CI      = 0x0205, // Congestion Indications
        CDST    = 0x0206, // Concerned Destination
        RKEY    = 0x0207, // Routing Key
        RRES    = 0x0208, // Registration Result
        DRES    = 0x0209, // Deregistration Result
        LRKI    = 0x020a, // Local_Routing Key Identifier
        DPC     = 0x020b, // Destination Point Code
        SI      = 0x020c, // Service Indicators
        OPCL    = 0x020e, // Originating Point Code List
        CR      = 0x020f, // Circuit Range
        PDATA   = 0x0210, // Protocol Data
        RSTATUS = 0x0212, // Registration Status
        DSTATUS = 0x0213, // Deregistration Status
    };

    uint16_t size()
    {
        return LS_ntohl(Sub::hdr()->length);
    }

    uint8_t* getWithOffset(uint16_t offset)
    {
        return (uint8_t*)Sub::layer()->get() + Sub::layer()->getOffset() + offset;
    }

    Sccp<M3ua<SUBLAYER>> makeSccp() { return move(Sccp<M3ua<SUBLAYER>>(move(*this))); }
    Isup<M3ua<SUBLAYER>> makeIsup() { return move(Isup<M3ua<SUBLAYER>>(move(*this))); }

    typedef HeaderIf<M3uaHdr, M3ua<SUBLAYER>, SUBLAYER> Sub;
    M3ua(SUBLAYER&& pkt) : Sub(move(pkt)) {}
};
