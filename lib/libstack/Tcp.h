// Copyright: https://github.com/mikerez/mediaroom/blob/main/LICENSE

#pragma once

#include "HeaderIf.h"

struct TcpHdr
{
    uint16_t source_port;
    uint16_t dest_port;
    uint32_t sequence;
    uint32_t acknowledge;
    uint8_t ns : 1;
    uint8_t reserved_part1 : 3;
    uint8_t data_offset : 4;
    uint8_t fin : 1;
    uint8_t syn : 1;
    uint8_t rst : 1;
    uint8_t psh : 1;
    uint8_t ack : 1;
    uint8_t urg : 1;
    uint8_t ecn : 1;
    uint8_t cwr : 1;
    uint16_t window;
    uint16_t checksum;
    uint16_t urgent_pointer;
};
BUILD_ASSERT(sizeof(struct TcpHdr) == 20);

template<class SUBLAYER>
struct Tcp : public HeaderIf<TcpHdr,Tcp<SUBLAYER>,SUBLAYER>
{
    enum Type
    {
    };

    uint16_t size()
    {
        return Sub::hdr()->data_offset<<2;
    }

    void setPayloadShift(uint16_t shift)
    {
        Sub::sublayerHdr()->payload_shift = shift;
    }

    uint16_t getPayloadShift()
    {
        return Sub::sublayerHdr()->payload_shift;
    }

    void addPayloadShift(uint16_t shift)
    {
        setPayloadShift(getPayloadShift() + shift);
    }

    uint8_t* payload()
    {
        LS_ASSERT(Sub::payloadLength() >= getPayloadShift());
        return Sub::payload() + getPayloadShift();
    }

    uint16_t payloadLength()
    {
        LS_ASSERT(Sub::payloadLength() >= getPayloadShift());
        return Sub::payloadLength() - getPayloadShift();
    }

    typedef HeaderIf<TcpHdr, Tcp<SUBLAYER>, SUBLAYER> Sub;
    Tcp(SUBLAYER&& pkt) : Sub(move(pkt)) {}
};
