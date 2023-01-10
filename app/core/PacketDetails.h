// Copyright: https://github.com/mikerez/mediaroom/blob/main/LICENSE
#pragma once

#include "Debug.h"
#include "Os.h"
#ifdef UNIT_TESTS
#include "FakeSystem.h"
#else
#include "System.h"
#endif
#include "Debug.h"
#include "LibStack.h"
#include "Eth.h"
#include "ss7/Mtp.h"
#include "ss7/Hdlc.h"
#include "Config.h"

struct PacketDetails
{
    enum class Type : uint8_t
    {
        Vlan,
        Mpls,
        _Bidir,  // bidir types start here
        Eth,
        Ip4,
        Tcp,
        Udp,
        Mtp, // ss7
        Mtp3,
        Sccp,
        Isup,
        Sctp,
        SctpChunk,
        M2ua,
        M3ua,
        MaxValue
    };


    static uint8_t getTypeLength(Type type)
    {
        switch (type) {
        case Type::Vlan: return 2;
        case Type::Mpls: return 2;
        case Type::Eth: return 12;
        case Type::Ip4: return 8;
        case Type::Tcp: return 4;
        case Type::Udp: return 4;
        case Type::Mtp: return 4;
        case Type::Mtp3: return 4;
        case Type::Sccp: return 1;
        case Type::Sctp: return 4;
        case Type::SctpChunk: return 0;
        case Type::M2ua: return 0;
        case Type::M3ua: return 0;
        case Type::Isup: return 1;
        default: RT_ASSERT(0);
            return -1;
        }
    }

    static uint32_t getTypeMask(Type type)
    {
        switch (type) {
        case Type::Vlan: return 0xFF/*todo*/;
        case Type::Mpls: return 0xFF/*todo*/;
        case Type::Eth: return 0;
        case Type::Ip4: return 0;
        case Type::Tcp: return 0;
        case Type::Udp: return 0;
        case Type::Sctp: return 0;
        case Type::SctpChunk: return 0;
        case Type::M2ua: return 0;
        case Type::M3ua: return 0;
        case Type::Mtp: return 0x3FFF;
        case Type::Mtp3: return 0x3FFF;
        case Type::Sccp: return 0;
        case Type::Isup: return 0;
        default: RT_ASSERT(0);
            return -1;
        }
    }

    struct AddrOffset
    {
        uint16_t typeOffset;
        Type getType() const
        {
            return (Type)(typeOffset & 0xF);
        }
        uint16_t getOffset() const
        {
            return typeOffset >> 5;
        }
        bool getMix() const
        {
            return (typeOffset & 0x10) >> 4;
        }
        void unsetMix()
        {
            typeOffset &= ~(1<<4);
        }
        void set(Type type, uint16_t offset, bool mix)
        {
            static_assert((uint8_t)Type::MaxValue <= 16, "Type is overflowed");
            RT_ASSERT((uint8_t)type < 16 && offset <= 0x3FFF);
            typeOffset = (offset << 5) + (mix << 4) + (uint8_t)type;

        }
    };

//    uint64_t srcHash;
//    uint64_t dstHash;
    uint64_t flowHash;
    uint8_t lastIpLayer;
    uint8_t lastMtpLayer;
    uint8_t lastM3uaLayer;
    uint8_t layersCnt;
    uint8_t layers2Cnt;
    uint8_t layers3Cnt;
    uint8_t layers4Cnt;
    bool more2Layers;
    bool more3Layers;
    bool more4Layers;
    mutable AddrOffset layers[1];

    static uint8_t layers2Max;
    static uint8_t layers3Max;
    static uint8_t layers4Max;

    static void configure(uint8_t l2Max, uint8_t l3Max, uint8_t l4Max)
    {
        layers2Max = l2Max;
        layers3Max = l3Max;
        layers4Max = l4Max;
    }

    void init()
    {
        lastIpLayer = lastMtpLayer = lastM3uaLayer = 0xFF;
        flowHash = layersCnt = layers2Cnt = layers3Cnt = layers4Cnt = more2Layers = more3Layers = more4Layers = 0;
    }

    template<class STACK>
    void pushEth(Eth<STACK>& eth, bool mix)
    {
        if (layers2Cnt != layers2Max) {
            if (!mix) flowHash += *((uint32_t*)&eth->source) + *((uint16_t*)&eth->source + 2);
            if (!mix) flowHash += *((uint32_t*)&eth->dest) + *((uint16_t*)&eth->dest + 2);
            layers[layersCnt++].set(Type::Eth, (uint16_t)((uint8_t*)&eth->source - (uint8_t*)eth.data()), mix);
            layers2Cnt++;
        }
        else {
            more2Layers = true;
        }
    }

    template<class STACK>
    void pushMtp(Mtp<STACK>& mtp)
    {
        flowHash += mtp->rl.opc;
        flowHash += mtp->rl.dpc;
        lastMtpLayer = layersCnt;
        if (layers3Cnt != layers3Max) {
            // ITU DPC and OPC have length 14 bits
            layers[layersCnt++].set(Type::Mtp, (uint16_t)((uint8_t*)&mtp->rl - (uint8_t*)mtp.data()), false);
            layers3Cnt++;
        }
        else {
            layers[layersCnt - 1].set(Type::Mtp, (uint16_t)((uint8_t*)&mtp->rl - (uint8_t*)mtp.data()), false);
            more3Layers = true;
        }
        LOG_DEBUG1(LOG_DETAILS, "pushMtp: 0x%s, flowHash: %lX, layersCnt: %d\n", Debug::printHex((uint8_t*)&mtp->rl, 4, Debug::TextBuffer<4>()), flowHash, layers3Cnt);
    }

    template<class STACK>
    void pushMtp3(Mtp3<STACK>& mtp3)
    {
        flowHash += mtp3->rl.opc;
        flowHash += mtp3->rl.dpc;
        lastMtpLayer = layersCnt;
        if (layers3Cnt != layers3Max) {
            // ITU DPC and OPC have length 14 bits
            layers[layersCnt++].set(Type::Mtp3, (uint16_t)((uint8_t*)&mtp3->rl - (uint8_t*)mtp3.data()), false);
            layers3Cnt++;
        }
        else {
            layers[layersCnt - 1].set(Type::Mtp3, (uint16_t)((uint8_t*)&mtp3->rl - (uint8_t*)mtp3.data()), false);
            more3Layers = true;
        }
        LOG_DEBUG1(LOG_DETAILS, "pushMtp: 0x%s, flowHash: %lX, layersCnt: %d\n", Debug::printHex((uint8_t*)&mtp3->rl, 4, Debug::TextBuffer<4>()), flowHash, layers3Cnt);
    }

    template<class STACK>
    void pushVlan(Vlan<STACK>& vlan, bool mix)
    {
        if (layers2Cnt != layers2Max) {
            if (!mix) flowHash += vlan->tci;
            if (!mix) flowHash += vlan->tci;
            layers[layersCnt++].set(Type::Vlan, (uint16_t)((uint8_t*)vlan.hdr() - (uint8_t*)vlan.data()), mix);
            layers2Cnt++;
        }
        else {
            more2Layers = true;
        }
    }

    template<class STACK>
    void pushMpls(Mpls<STACK>& mpls, bool mix)
    {
        if (layers2Cnt != layers2Max) {
            if (!mix) flowHash += mpls->getLabel();
            if (!mix) flowHash += mpls->getLabel();
            layers[layersCnt++].set(Type::Mpls, (uint16_t)((uint8_t*)mpls.hdr() - (uint8_t*)mpls.data()), mix);
            layers2Cnt++;
        }
        else {
            more2Layers = true;
        }
    }

    template<class STACK>
    void pushIp4(Ip4<STACK>& ip, bool mix /*need to mix some VPNs?*/)
    {
        if (layers3Cnt != layers3Max) {
            if (!mix) flowHash += ip->srcaddr;
            if (!mix) flowHash += ip->dstaddr;
            lastIpLayer = layersCnt;
            layers[layersCnt++].set(Type::Ip4, (uint16_t)((uint8_t*)&ip->srcaddr - (uint8_t*)ip.data()), mix);
            layers3Cnt++;
        }
        else {  // always use last IP header
            flowHash += ip->srcaddr;
            flowHash += ip->dstaddr;
            layers[layersCnt - 1].set(Type::Ip4, (uint16_t)((uint8_t*)&ip->srcaddr - (uint8_t*)ip.data()), false);
            more3Layers = true;
        }
    }

    template<class STACK>
    void pushIsup(Isup<STACK>& isup)
    {
        flowHash += isup->cic.cic;
        if (layers4Cnt != layers4Max) {
            layers[layersCnt++].set(Type::Isup, (uint16_t)((uint8_t*)&isup->cic - (uint8_t*)isup.data()), false);
            layers4Cnt++;
        }
        else {  // always use last TCP header
            layers[layersCnt - 1].set(Type::Isup, (uint16_t)((uint8_t*)&isup->cic - (uint8_t*)isup.data()), false);
            more4Layers = true;
        }
        LOG_DEBUG1(LOG_DETAILS, "pushIsup: 0x%s, flowHash: %lX, layersCnt: %d\n", Debug::printHex((uint8_t*)&isup->cic, 2, Debug::TextBuffer<2>()), flowHash, layers3Cnt);
    }

    template<class STACK>
    void pushTcp(Tcp<STACK>& tcp)
    {
        if (layers4Cnt != layers4Max) {
            flowHash += tcp->source_port;
            flowHash += tcp->dest_port;
            layers[layersCnt++].set(Type::Tcp, (uint16_t)((uint8_t*)&tcp->source_port - (uint8_t*)tcp.data()), false);
            layers4Cnt++;
        }
        else {  // always use last TCP header
            flowHash += tcp->source_port;
            flowHash += tcp->dest_port;
            layers[layersCnt - 1].set(Type::Tcp, (uint16_t)((uint8_t*)&tcp->source_port - (uint8_t*)tcp.data()), false);
            more4Layers = true;
        }
    }

    template<class STACK>
    void pushUdp(Udp<STACK>& udp)
    {
        if (layers4Cnt != layers4Max) {
            flowHash += udp->source_port;
            flowHash += udp->dest_port;
            layers[layersCnt++].set(Type::Udp, (uint16_t)((uint8_t*)&udp->source_port - (uint8_t*)udp.data()), false);
            layers4Cnt++;
        }
        else {  // always use last TCP header
            flowHash += udp->source_port;
            flowHash += udp->dest_port;
            layers[layersCnt - 1].set(Type::Udp, (uint16_t)((uint8_t*)&udp->source_port - (uint8_t*)udp.data()), false);
            more4Layers = true;
        }
    }

    template<class STACK>
    void pushSctp(Sctp<STACK>& sctp)
    {
        if (layers4Cnt != layers4Max) {
            flowHash += sctp->source_port;
            flowHash += sctp->dest_port;
            layers[layersCnt++].set(Type::Sctp, (uint16_t)((uint8_t*)&sctp->source_port - (uint8_t*)sctp.data()), false);
            layers4Cnt++;
        }
        else {
            flowHash += sctp->source_port;
            flowHash += sctp->dest_port;
            layers[layersCnt - 1].set(Type::Sctp, (uint16_t)((uint8_t*)&sctp->source_port - (uint8_t*)sctp.data()), false);
            more4Layers = true;
        }
    }

    template<class STACK>
    void pushSctpChunk(SctpChunk<STACK>& chunk)
    {
        if (layers4Cnt != layers4Max) {
            layers[layersCnt++].set(Type::SctpChunk, (uint16_t)((uint8_t*)&chunk->type - (uint8_t*)chunk.data()), false);
        }
    }

    template<class STACK>
    void pushM2ua(M2ua<STACK>& m2ua)
    {
        if (layers4Cnt != layers4Max) {
            layers[layersCnt++].set(Type::M2ua, (uint16_t)((uint8_t*)&m2ua->version - (uint8_t*)m2ua.data()), false);
        }
    }

    template<class STACK>
    void pushM3ua(M3ua<STACK>& m3ua, uint16_t opcOffset = 0)
    {
        lastM3uaLayer = layersCnt;
        if (opcOffset) {
            flowHash += *(uint32_t *)m3ua.getWithOffset(opcOffset); // LS_ntohl?
            flowHash += *(uint32_t *)m3ua.getWithOffset(opcOffset + 4); // dpc
        }

        if (layers4Cnt != layers4Max) {
            layers[layersCnt++].set(Type::M3ua, (uint16_t)((uint8_t*)&(m3ua->version) + opcOffset - (uint8_t*)m3ua.data()), false);
            layers4Cnt++;
        }
    }

    uint8_t getLayersCnt() const
    {
        RT_ASSERT(layers2Cnt <= layers2Max && layers3Cnt <= layers3Max && layers4Cnt <= layers4Max);
        if (lastIpLayer != 0xFF) {
            layers[lastIpLayer].unsetMix();  // last IP layer is always valuable
        }
        return layers2Cnt + layers3Cnt + layers4Cnt;
    }

    const uint8_t* getLayerAddr(const uint8_t* data, uint8_t layer, uint8_t& len, Type& type, uint32_t& mask, bool& mix) const
    {
        RT_ASSERT(layer < layers2Cnt + layers3Cnt + layers4Cnt);
        type = layers[layer].getType();
        mix = layers[layer].getMix();
        len = getTypeLength(type);
        mask = getTypeMask(type);
        return data + layers[layer].getOffset();
    }

    bool isSrcLowerAddr(const uint8_t* data) const
    {
        int layersCnt = getLayersCnt();
        uint8_t pos = 0;
        for (int i = (int)layersCnt - 1; i >= 0; i--) {
            uint8_t len;
            bool mix;
            PacketDetails::Type type;
            uint32_t mask;
            const uint8_t* addrPtr = getLayerAddr(data, i, len, type, mask, mix);
            //LOG_DEBUG(LOG_DETAILS, "isSrcLowerAddr: 0x%s, mix: %d, type: %d, mask: %x\n", Debug::printHex(addrPtr, len, Debug::TextBuffer<32>()), (int)mix, (int)type, (int)mask);
            if (type > PacketDetails::Type::_Bidir && !mix) {
                if (pos + len > CONFIG_FLOWADDR_SIZE - 1) {
                    break;  // too many layers for CONFIG_FLOWADDR_SIZE, count stats
                }
                if (!mask) {
                    RT_ASSERT(len == 1 || (len % 2) == 0);
                    int res = memcmp(addrPtr, addrPtr + len / 2, len / 2);
                    if (res < 0) {
                        return true;
                    }
                    if (res > 0) {
                        return false;
                    }
                }
                else {
                    unsigned indStart, indEnd;
                    Os::bitScanForward(&indStart, (uint32_t)mask);
                    Os::bitScanReverse(&indEnd, (uint32_t)mask);
                    uint32_t shiftedMask = mask >> indStart;
                    uint32_t srcAddr = ((*(uint32_t*)addrPtr) >> indStart) & shiftedMask;
                    uint32_t dstAddr = ((*(uint32_t*)addrPtr) >> indEnd) & shiftedMask;

                    //LOG_DEBUG(LOG_DETAILS, "isSrcLowerAddr: %X, srcAddr: %X, dstAddr: %X, indStart: %d, indEnd: %d\n", (int)shiftedMask, (int)srcAddr, (int)dstAddr, (int)indStart, (int)indEnd);
                    if (srcAddr < dstAddr) {
                        return true;
                    }
                    if (srcAddr > dstAddr) {
                        return false;
                    }
                }
                pos += len;
            }
        }
        return false; // equality case
    }

    uint8_t fillAddresses(const uint8_t* packetData, uint8_t* addresses, bool side) const
    {
        int layersCnt = getLayersCnt();
        uint8_t pos = 0;
        for (int i = layersCnt - 1; i >= 0; i--) {
            uint8_t len;
            bool mix;
            PacketDetails::Type type;
            uint32_t mask;
            const uint8_t* addrPtr = getLayerAddr(packetData, i, len, type, mask, mix);
            LOG_DEBUG(LOG_DETAILS, "fillAddresses: 0x%s, mix: %d, type: %d, mask %x\n", Debug::printHex(addrPtr, len, Debug::TextBuffer<32>()), (int)mix, (int)type, (int)mask);
            if (!mix) {
                if (pos + len > CONFIG_FLOWADDR_SIZE - 1) {
                    break;  // too many layers for CONFIG_FLOWADDR_SIZE, count stats
                }
                if (!mask) {
                    RT_ASSERT(len == 1 || (len % 2) == 0);
                    if (side || len == 1) {  // src is lower
                        memcpy(addresses + pos, addrPtr, len);
                    }
                    else {
                        memcpy(addresses + pos, addrPtr + len / 2, len / 2);
                        memcpy(addresses + pos + len / 2, addrPtr, len / 2);
                    }
                }
                else {
                    unsigned indStart, indEnd;
                    Os::bitScanForward(&indStart, (uint32_t)mask);
                    Os::bitScanReverse(&indEnd, (uint32_t)mask);
                    uint32_t shiftedMask = mask >> indStart;
                    uint32_t srcAddr = ((*(uint32_t*)addrPtr) >> indStart) & shiftedMask;
                    uint32_t dstAddr = ((*(uint32_t*)addrPtr) >> indEnd) & shiftedMask;

                    LOG_DEBUG(LOG_DETAILS, "fillAddresses: %X, srcAddr: %X, dstAddr: %X, side: %d, indStart: %d, indEnd: %d\n", (int)shiftedMask, (int)srcAddr, (int)dstAddr, (int)side, (int)indStart, (int)indEnd);
                    if (side) {  // src is lower
                        memcpy(addresses + pos, &srcAddr, len / 2);
                        memcpy(addresses + pos + len / 2, &dstAddr, len / 2);
                    }
                    else {
                        memcpy(addresses + pos, &dstAddr, len / 2);
                        memcpy(addresses + pos + len / 2, &srcAddr, len / 2);
                    }
                }
                pos += len;
            }
        }
        addresses[CONFIG_FLOWADDR_SIZE - 1] = pos;
        return pos;
    }

    bool calcSide(const uint8_t* packetData) const
    {
        int layersCnt = getLayersCnt();
        uint8_t pos = 0;
        int ret;
        int i = (int)layersCnt - 1;
        for (; i >= 0; i--) {
            uint8_t len;
            bool mix;
            PacketDetails::Type type;
            uint32_t mask;  // TODO: using mask for VLAN/MPLS if !mix
            const uint8_t* addrPtr = getLayerAddr(packetData, i, len, type, mask, mix);
            LOG_DEBUG(LOG_DETAILS, "calcSide: 0x%s, mix: %d, type: %d, mask: %x\n", Debug::printHex(addrPtr, len, Debug::TextBuffer<32>()), (int)mix, (int)type, (int)mask);
            if (!mix) {
                if (pos + len > CONFIG_FLOWADDR_SIZE - 1) {
                    break;  // too many layers for CONFIG_FLOWADDR_SIZE, count stats
                }
                if (!mask) {
                    RT_ASSERT(len == 1 || (len % 2) == 0);
                    ret = memcmp(addrPtr, addrPtr + len / 2, len / 2);
                    if (ret < 0) {
                        return 0;
                    }
                    if (ret > 0) {
                        return 1;
                    }
                }
                else {
                    unsigned indStart, indEnd;
                    Os::bitScanForward(&indStart, (uint32_t)mask);
                    Os::bitScanReverse(&indEnd, (uint32_t)mask);
                    uint32_t shiftedMask = mask >> indStart;
                    uint32_t srcAddr = ((*(uint32_t*)addrPtr) >> indStart) & shiftedMask;
                    uint32_t dstAddr = ((*(uint32_t*)addrPtr) >> indEnd) & shiftedMask;

                    LOG_DEBUG(LOG_DETAILS, "calcSide: %X, srcAddr: %X, dstAddr: %X, indStart: %d, indEnd: %d\n", (int)shiftedMask, (int)srcAddr, (int)dstAddr, (int)indStart, (int)indEnd);
                    if (srcAddr < dstAddr) {
                        return 0;
                    }
                    if (srcAddr > dstAddr) {
                        return 1;
                    }
                }
                pos += len;
            }
        }
        return 0; // equality case
    }

    bool cmpAddresses(const uint8_t* packetData, uint8_t* addresses, bool side) const
    {
        int layersCnt = getLayersCnt();
        uint8_t pos = 0;
        int i = (int)layersCnt - 1;
        for (; i >= 0; i--) {
            uint8_t len = 0;
            bool mix;
            PacketDetails::Type type;
            uint32_t mask;  // TODO: using mask for VLAN/MPLS if !mix
            const uint8_t* addrPtr = getLayerAddr(packetData, i, len, type, mask, mix);
            LOG_DEBUG(LOG_DETAILS, "cmpAddresses: 0x%s, mix: %d, type: %d, mask: %x\n", Debug::printHex(addrPtr, len, Debug::TextBuffer<32>()), (int)mix, (int)type, (int)mask);
            if (!mix) {
                if (pos + len > CONFIG_FLOWADDR_SIZE - 1) {
                    break;  // too many layers for CONFIG_FLOWADDR_SIZE, count stats
                }
                if (!mask) {
                    RT_ASSERT(len == 1 || (len % 2) == 0);
                    if (side || len == 1) {
                        if (memcmp(addresses + pos, addrPtr, len) != 0) {
                            return false;
                        }
                    }
                    else {  // swap addresses
                        if (memcmp(addresses + pos, addrPtr + len / 2, len / 2) != 0) {
                            return false;
                        }
                        if (memcmp(addresses + pos + len / 2, addrPtr, len / 2) != 0) {
                            return false;
                        }
                    }
                }
                else {
                    unsigned indStart, indEnd;
                    Os::bitScanForward(&indStart, (uint32_t)mask);
                    Os::bitScanReverse(&indEnd, (uint32_t)mask);
                    uint32_t shiftedMask = mask >> indStart;
                    uint32_t srcAddr = ((*(uint32_t*)addrPtr) >> indStart) & shiftedMask;
                    uint32_t dstAddr = ((*(uint32_t*)addrPtr) >> indEnd) & shiftedMask;

                    LOG_DEBUG(LOG_DETAILS, "cmpAddresses: %X, srcAddr: %X, dstAddr: %X, side: %d, indStart: %d, indEnd: %d\n", (int)shiftedMask, (int)srcAddr, (int)dstAddr, (int)side, (int)indStart, (int)indEnd);
                    if (side) {  // src is lower
                        if (memcmp(addresses + pos, &srcAddr, len / 2) != 0) {
                            return false;
                        }
                        if (memcmp(addresses + pos + len / 2, &dstAddr, len / 2) != 0) {
                            return false;
                        }
                    }
                    else {  // swap addresses
                        if (memcmp(addresses + pos, &dstAddr, len / 2) != 0) {
                            return false;
                        }
                        if (memcmp(addresses + pos + len / 2, &srcAddr, len / 2) != 0) {
                            return false;
                        }
                    }
                }
                pos += len;
            }
        }
        return addresses[CONFIG_FLOWADDR_SIZE - 1] == pos;  // all addressess are equal
    }

    const uint8_t* getIpAddr(const uint8_t* packetData, bool src, Type& typeOut) const
    {
        if (lastIpLayer != 0xFF) {
            uint8_t len;
            bool mix;
            PacketDetails::Type type;
            uint32_t mask;  // TODO: using mask for VLAN/MPLS if !mix
            const uint8_t* addrPtr = getLayerAddr(packetData, lastIpLayer, len, type, mask, mix);
            typeOut = type;
            return addrPtr + (src?0:len/2);
        }
        else {
            return nullptr;
        }
    }

    const uint8_t* getPort(const uint8_t* packetData, bool src, Type& typeOut) const
    {
        if (typeOut == PacketDetails::Type::M3ua)
        {
            if (lastM3uaLayer != 0xFF && lastM3uaLayer - 1 > 1) {
                uint8_t len;
                bool mix;
                PacketDetails::Type type;
                uint32_t mask;  // TODO: using mask for VLAN/MPLS if !mix
                const uint8_t* addrPtr = getLayerAddr(packetData, lastM3uaLayer - 1, len, type, mask, mix);
                return addrPtr + (src ? 0 : 2);
            }
            else
                return nullptr; // Case MTP3->SCCP->TCAP->INAP has no ports
        }

        int layersCnt = getLayersCnt();
        if (lastIpLayer != 0xFF && layersCnt > lastIpLayer + 1) {
            uint8_t len;
            bool mix;
            PacketDetails::Type type;
            uint32_t mask;  // TODO: using mask for VLAN/MPLS if !mix
            const uint8_t* addrPtr = getLayerAddr(packetData, lastIpLayer + 1, len, type, mask, mix);
            typeOut = type;
            return addrPtr + (src ? 0 : len / 2);
        }
        else {
            return nullptr;
        }
    }
    const routing_label_t* getRl(const uint8_t* packetData, Type& typeOut) const
    {
        if (lastMtpLayer != 0xFF) {
            uint8_t len;
            bool mix;
            PacketDetails::Type type;
            uint32_t mask;
            const uint8_t* addrPtr = getLayerAddr(packetData, lastMtpLayer, len, type, mask, mix);
            typeOut = type;
            return (const routing_label_t *)addrPtr;
        }
        else {
            return nullptr;
        }
    }

    const cic_t* getCic(const uint8_t* packetData, Type& typeOut) const
    {
        int layersCnt = getLayersCnt();
        if (lastMtpLayer != 0xFF && layersCnt > lastMtpLayer + 1) {
            uint8_t len;
            bool mix;
            PacketDetails::Type type;
            uint32_t mask;  // TODO: using mask for VLAN/MPLS if !mix
            const uint8_t* addrPtr = getLayerAddr(packetData, lastMtpLayer + 1, len, type, mask, mix);
            typeOut = type;
            return (const cic_t*)addrPtr;
        }
        else {
            return nullptr;
        }
    }

    uint32_t getPc(const uint8_t* packetData, bool src, Type& typeOut) const
    {
        uint8_t len;
        bool mix;
        PacketDetails::Type type;
        uint32_t mask;  // TODO: using mask for VLAN/MPLS if !mix
        uint8_t layer;

        if (lastM3uaLayer != 0xFF)
            layer = lastM3uaLayer;
        else if (lastMtpLayer != 0xFF)
            layer = lastMtpLayer;
        else
            return 0;

        const uint8_t* addrPtr = getLayerAddr(packetData, layer, len, type, mask, mix);
        typeOut = type;

        if (lastM3uaLayer != 0xFF)
            return LS_ntohl(*(const uint32_t*)(addrPtr + ( src ? 0 : 4 )));
        else if (lastMtpLayer != 0xFF)
            return LS_ntohs(*(const uint16_t*)(addrPtr + (src ? 0 : 2)));
        else
            return 0;
    }

    struct AddrBuffer
    {
        enum {
            Size = 64
        };
        char buffer[Size];
    };

    const char* printSrcAddr(const uint8_t* packetData, AddrBuffer&& addrBuff) const
    {
        int length = 0;
        char* buffer = addrBuff.buffer;
        buffer[0] = 0;
        uint8_t tcpLayer = 0xFF;
        uint8_t ip4Layer = 0xFF;
        uint8_t ip6Layer = 0xFF;
        // SS7
        uint8_t mtpLayer  = 0xFF;
        uint8_t m3uaLayer = 0xFF;
        uint8_t sccpLayer = 0xFF;
        uint8_t isupLayer = 0xFF;

        uint8_t layer = layersCnt - 1;
        if (layer == 0xFF) return buffer;
        Type type = layers[layer].getType();
        if (type == Type::Tcp || type == Type::Udp) {
            tcpLayer = layer;
            --layer;
        }
        if (type == Type::Isup) {
            isupLayer = layer;
            --layer;
        }
        if (layer != 0xFF) {
            type = layers[layer].getType();
            if (type == Type::Ip4) {
                ip4Layer = layer;
                --layer;
            }
            if (type == Type::Sccp) {
                sccpLayer = layer;
                --layer;
            }
            if (type == Type::Mtp) {
                mtpLayer = layer;
            }
            if (type == Type::M3ua) {
                m3uaLayer = layer;
                --layer;
            }
        }
 /*       if (layer != 0xFF) {
            type = layers[layer].getType();
            if (type == Type::Ip6) {
                ip4Layer = layer;
                --layer;
            }
        }*/
        if (ip4Layer != 0xFF) {
            uint32_t ip = *(uint32_t*)(packetData + layers[ip4Layer].getOffset());
            length += snprintf(buffer + length, AddrBuffer::Size - length, "%d.%d.%d.%d", (ip & 0xFF),
                (ip & 0xFF00) >> 8, (ip & 0xFF0000) >> 16, (ip & 0xFF000000) >> 24);
        }
        if (mtpLayer != 0xFF) {
            routing_label_t rl = *(routing_label_t*)(packetData + layers[mtpLayer].getOffset());
            length += snprintf(buffer + length, AddrBuffer::Size - length, "%d", rl.opc);
        }
        if (tcpLayer != 0xFF) {
            uint32_t port = *(uint16_t*)(packetData + layers[tcpLayer].getOffset());
            length += snprintf(buffer + length, AddrBuffer::Size - length, ":%u", LS_ntohs(port));
        }
        if (m3uaLayer != 0xFF) { // port from sctp
            uint32_t port = *(uint16_t*)(packetData + layers[layer].getOffset());
            length += snprintf(buffer + length, AddrBuffer::Size - length, ":%d", LS_ntohs(port));
        }
        return buffer;
    }

    const char* printDstAddr(const uint8_t* packetData, AddrBuffer&& addrBuff) const
    {
        int length = 0;
        char* buffer = addrBuff.buffer;
        buffer[0] = 0;
        uint8_t tcpLayer = 0xFF;
        uint8_t ip4Layer = 0xFF;
        uint8_t ip6Layer = 0xFF;
        // SS7
        uint8_t mtpLayer = 0xFF;
        uint8_t m3uaLayer = 0xFF;
        uint8_t sccpLayer = 0xFF;
        uint8_t isupLayer = 0xFF;

        uint8_t layer = layersCnt - 1;
        if (layer == 0xFF) return buffer;
        Type type = layers[layer].getType();
        if (type == Type::Tcp || type == Type::Udp) {
            tcpLayer = layer;
            --layer;
        }
        if (type == Type::Isup) {
            isupLayer = layer;
            --layer;
        }
        if (layer != 0xFF) {
            type = layers[layer].getType();
            if (type == Type::Ip4) {
                ip4Layer = layer;
                --layer;
            }
            if (type == Type::Sccp) {
                sccpLayer = layer;
                --layer;
            }
            if (type == Type::Mtp) {
                mtpLayer = layer;
            }
            if (type == Type::M3ua) {
                m3uaLayer = layer;
                --layer;
            }
        }
        /*       if (layer != 0xFF) {
        type = layers[layer].getType();
        if (type == Type::Ip6) {
        ip4Layer = layer;
        --layer;
        }
        }*/
        if (ip4Layer != 0xFF) {
            uint32_t ip = *(uint32_t*)(packetData + layers[ip4Layer].getOffset() + 4);
            length += snprintf(buffer + length, AddrBuffer::Size - length, "%d.%d.%d.%d", (ip & 0xFF),
                (ip & 0xFF00) >> 8, (ip & 0xFF0000) >> 16, (ip & 0xFF000000) >> 24);
        }
        if (mtpLayer != 0xFF) {
            routing_label_t rl = *(routing_label_t*)(packetData + layers[mtpLayer].getOffset());
            length += snprintf(buffer + length, AddrBuffer::Size - length, "%d", rl.dpc);
        }
        if (tcpLayer != 0xFF) {
            uint32_t port = *(uint16_t*)(packetData + layers[tcpLayer].getOffset() + 2);
            length += snprintf(buffer + length, AddrBuffer::Size - length, ":%d", LS_ntohs(port));
        }
        if (m3uaLayer != 0xFF) { // port from sctp
            uint32_t port = *(uint16_t*)(packetData + layers[layer].getOffset() + 2);
            length += snprintf(buffer + length, AddrBuffer::Size - length, ":%d", LS_ntohs(port));
        }
        return buffer;
    }

    const char* printSlsCic(const uint8_t* packetData, AddrBuffer&& addrBuff) const
    {
        int length = 0;
        char* buffer = addrBuff.buffer;
        buffer[0] = 0;
        // SS7
        uint8_t mtpLayer = 0xFF;
        uint8_t sccpLayer = 0xFF;
        uint8_t isupLayer = 0xFF;

        uint8_t layer = layersCnt - 1;
        if (layer == 0xFF) return buffer;
        Type type = layers[layer].getType();
        if (type == Type::Isup) {
            isupLayer = layer;
            --layer;
        }
        if (layer != 0xFF) {
            type = layers[layer].getType();
            if (type == Type::Sccp) {
                sccpLayer = layer;
                --layer;
            }
            if (type == Type::Mtp) {
                mtpLayer = layer;
            }
        }
        if (mtpLayer != 0xFF) {
            routing_label_t rl = *(routing_label_t*)(packetData + layers[mtpLayer].getOffset());
            length += snprintf(buffer + length, AddrBuffer::Size - length, "%d", rl.sls);
        }
        if (isupLayer != 0xFF) {
            cic_t cic = *(cic_t*)(packetData + layers[isupLayer].getOffset());
            length += snprintf(buffer + length, AddrBuffer::Size - length, ":%d", cic.cic);
        }
        return buffer;
    }
};

inline size_t calcPacketDetailsSize(int layers)
{
    return sizeof(PacketDetails) + sizeof(PacketDetails::AddrOffset)*(layers - 1/*already present in struct*/);
}
