// Copyright: https://github.com/mikerez/mediaroom/blob/main/LICENSE

#pragma once

#include "HeaderIf.h"

#pragma pack(push, 1)

/*
 * Q.763
 * 1.2 Circuit identification code
 * The format of the Circuit Identification Code (CIC) is shown in Figure 2.
 */
typedef struct Cic {
    uint16_t cic : 12;
    uint16_t spare : 4;
} cic_t;

struct IsupHdr
{
    cic_t cic;
};

#pragma pack(pop)

BUILD_ASSERT(sizeof(struct IsupHdr) == 2);

template<class SUBLAYER>
struct Isup : public HeaderIf<IsupHdr,Isup<SUBLAYER>,SUBLAYER>
{
    typedef HeaderIf<IsupHdr, Isup<SUBLAYER>, SUBLAYER> Sub;
    Isup(SUBLAYER&& pkt) : Sub(move(pkt)) {}
};
