#include "DriverPcap.h"

void DriverPcap::pcapHandler(u_char *user, const struct pcap_pkthdr *pkt_header, const u_char *pkt_data)
{
    if (pkt_header->caplen >= 65536)
    {
        LOG_MESS(DEBUG_DRIVER, "LinPcapDriver: got very big packet: %u bytes\n", pkt_header->caplen);
        return; // ignore this packet
    }

    DriverPcap* drv = reinterpret_cast<DriverPcap*>(user);
    Packet *pkt = allocPacket(pkt_header->caplen);
    memcpy(pkt->data(), pkt_data, pkt_header->caplen);
    pkt->caplen = static_cast<uint16_t>(pkt_header->caplen);
    pkt->length = static_cast<uint16_t>(pkt_header->caplen);

    drv->pcap_driver_stat.rx_bytes += static_cast<uint16_t>(pkt_header->caplen);
    drv->pcap_driver_stat.rx_packs++;

    int dl = drv->datalink();
    switch (dl)
    {
    case DLT_MTP2:
        pkt->type = Packet::Type::L2Mtp;
        break;
    case DLT_MTP3:
        pkt->type = Packet::Type::L2Mtp3;
        break;
    case DLT_LAPD:
        pkt->type = Packet::Type::L2Lapd;
        break;
    default:
        pkt->type = Packet::Type::L2Eth;
        break;
    }

    if (drv->_offline && drv->offlineProcess(pkt, pkt_header->ts))
        return;

    *(drv->_currBulk + drv->_currPkts) = pkt;
    drv->_currPkts++;
}
