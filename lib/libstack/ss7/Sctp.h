// Copyright: https://github.com/mikerez/mediaroom/blob/main/LICENSE

#pragma once

#include "HeaderIf.h"
#include "M2ua.h"
#include "M3ua.h"

struct SctpHdr
{
    uint16_t source_port;
    uint16_t dest_port;
    uint32_t verification_tag;
    uint32_t checksum;
};
BUILD_ASSERT(sizeof(struct SctpHdr) == 12);

struct ChunkHdr
{
    uint8_t type;
    uint8_t flags;
    uint16_t length;
};

struct ChunkData
{
    ChunkHdr hdr;
    uint32_t tsn;       // Transmission sequence number
    uint16_t stream_id; // Stream identifier
    uint16_t stream_sn; // Stream sequence number
    uint32_t proto;     // Payload protocol identifier
};

template<class SUBLAYER>
struct SctpChunkData : public HeaderIf<ChunkData, SctpChunkData<SUBLAYER>, SUBLAYER>
{
    enum Proto
    {
        SctpM2ua = 0x02000000,
        SctpM3ua = 0x03000000,
    };

    M2ua<SctpChunkData<SUBLAYER>> makeM2ua() { return move(M2ua<SctpChunkData<SUBLAYER>>(move(*this))); }
    M3ua<SctpChunkData<SUBLAYER>> makeM3ua() { return move(M3ua<SctpChunkData<SUBLAYER>>(move(*this))); }

    uint16_t size()
    {
        return sizeof(ChunkData);
    }

    typedef HeaderIf<ChunkData, SctpChunkData<SUBLAYER>, SUBLAYER> Sub;
    SctpChunkData(SUBLAYER&& pkt) : Sub(move(pkt)) {}
};

template<class SUBLAYER>
struct SctpChunk : public HeaderIf<ChunkHdr, SctpChunk<SUBLAYER>, SUBLAYER>
{
    enum Type : uint8_t
    {
        SCTP_DATA          = 0,       // Payload Data
        SCTP_INIT          = 0x1,     // Initiation
        SCTP_INIT_ACK      = 0x2,     // Initiation Acknowledgement
        SCTP_SACK          = 0x3,     // Selective Acknowledgement
        SCTP_HEARTBEAT     = 0x4,     // Heartbeat Request
        SCTP_HEARTBEAT_ACK = 0x5,     // Heartbeat Acknowledgement
        SCTP_ABORT         = 0x6,     // Abort
        SCTP_SHUTDOWN      = 0x7,     // Shutdown
        SCTP_SHUTDOWN_ACK  = 0x8,     // Shutdown Acknowledgement
        SCTP_ERROR         = 0x9,     // Operation Error
        SCTP_COOKIE_ECHO   = 0xa,     // State Cookie
        SCTP_COOKIE_ACK    = 0xb,     // Cookie Acknowledgement
        SCTP_ECNE          = 0xc,     // Reserved for Explicit Congestion Notification Echo
        SCTP_CWR           = 0xd,     // Reserved for Congestion Window Reduced
        SCTP_SHUTDOWN_COMPLETE = 0xe, // Shutdown Complete
    };

    SctpChunk<SctpChunk<SUBLAYER>> makeSctpChunk() { return move(SctpChunk<SctpChunk<SUBLAYER>>(move(*this))); }
    SctpChunkData<SctpChunk<SUBLAYER>> makeSctpChunkData() { return move(SctpChunkData<SctpChunk<SUBLAYER>>(move(*this))); }

    uint16_t size()
    {
        return LS_ntohs(Sub::hdr()->length);
    }

    typedef HeaderIf<ChunkHdr, SctpChunk<SUBLAYER>, SUBLAYER> Sub;
    SctpChunk(SUBLAYER&& pkt) : Sub(move(pkt)) {}
};

template<class SUBLAYER>
struct Sctp : public HeaderIf<SctpHdr, Sctp<SUBLAYER>, SUBLAYER>
{
    SctpChunk<Sctp<SUBLAYER>> makeSctpChunk() { return move(SctpChunk<Sctp<SUBLAYER>>(move(*this))); }
    Sctp<Sctp<SUBLAYER>> makeSctp() { return move(Sctp<Sctp<SUBLAYER>>(move(*this))); }

    typedef HeaderIf<SctpHdr, Sctp<SUBLAYER>, SUBLAYER> Sub;
    Sctp(SUBLAYER&& pkt) : Sub(move(pkt)) {}
};
