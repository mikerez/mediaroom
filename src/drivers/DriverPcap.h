#pragma once

#include "pcap.h"
#include "time.h"

#include "Driver.h"
#include "Debug.h"

#include <string>
#include <sstream>

class DriverPcap: public Driver, private DriverOffline
{
public:
    DriverPcap(std::string iface, int mtu) : Driver()
    {
        this->iface = iface;
        char errbuf[PCAP_ERRBUF_SIZE];
        if (iface.find("file=") == 0) {
            if ((_handle = pcap_open_offline(iface.c_str() + 5, errbuf)) == NULL) {
                std::ostringstream err;
                err << "cant open PCAP file: '" << (iface.c_str() + 5) << errbuf << "\n";
                throw std::runtime_error(err.str());
            }
            _offline = true;
        }
        else {
            if ((_handle = pcap_open_live(iface.c_str(),          // name of the device
                mtu,  // portion of the packet to capture
                      // 65536 guarantees that the whole packet will be captured on all the link layers
                1,  // promiscuous mode
                1,  // read timeout
                errbuf  // error buffer
            )) == NULL)
            {
                listDevs(errbuf);
                std::ostringstream err;
                err << "cant open PCAP iface: '" << iface << "', mtu: " << mtu << ", err: " << errbuf << "\n" << listDevs(errbuf);
                throw std::runtime_error(err.str());
            }
        }

        gettimeofday(&prev, nullptr);
        pcap_set_buffer_size(_handle, 2 * 1024 * 1024 * 300);
    }

    ~DriverPcap()
    {
        pcap_close(_handle);
    }

    std::string listDevs(char* errbuf)
    {
        std::ostringstream message;
        pcap_if_t *alldevs;
        pcap_if_t *d;
        int i = 0;

        message << "Available interfaces: \n";

        /* Retrieve the device list from the local machine */
        if (pcap_findalldevs(&alldevs, errbuf) == -1)
        {
            return message.str();
        }

        /* Print the list */
        for (d = alldevs; d != NULL; d = d->next)
        {
            message << ++i << ": " << d->name;
            if (d->description)
                message << " (" << d->description << ")";
            else
                message << " (No description available)\n";
            message << "\n";
        }

         /* We don't need any more the device list. Free it */
        pcap_freealldevs(alldevs);
        return message.str();
    }

    static void pcapHandler(u_char *user, const struct pcap_pkthdr *pkt_header, const u_char *pkt_data);

    size_t getPackets(Packet** bulk, size_t bulkLimit) override
    {
        _currPkts = 0;
        _currBulk = bulk;
//        pcap_breakloop(_handle);
        if (offlinePkt()) {
            RT_ASSERT(_offline);
            _currPkts += offlineHandle(_currBulk + _currPkts);
        } else {
            if (pcap_dispatch(_handle, _offline ? 1 : bulkLimit, pcapHandler, reinterpret_cast<u_char*>(this)) < 0) {
                //throw std::logic_error("error during pcap_dispatch");
                return 0;
            }
            if (_offline && _currPkts == 0 && !offlinePkt()) {
                _finished = true;
                static bool printed_finished = false;
                if(!printed_finished) {
                    printf("Pcap parsing finished!\n");
                    fflush(stdout);
                    printed_finished = true;
                }
            }
        }
        return _currPkts;
    }

    bool finished()
    {
        return _finished;
    }
    int datalink()
    {
        return pcap_datalink(_handle);
    }

    void idle(unsigned id) {
        struct timeval curr;
        gettimeofday(&curr, nullptr);
        auto diff = TimeHandler::timeval_diff(curr, prev);

        if(diff > 500000) {
            pcap_stat stat;
            auto res = pcap_stats(_handle, &stat);
            auto speed = 8*(pcap_driver_stat.rx_bytes - pcap_driver_stat.rx_bytes_prev)/static_cast<double>(diff);

            if(res == 0) {
                FILE * file = fopen(std::string("./libpcap_stat_" + iface + ".txt").c_str(), "w");
                if(file) {
                    fprintf(file, "(LibPcap %s ONLINE) recv: %i, drop: %i rx_bytes: %lu(%.2f Mbit/s) rx_packs: %lu, ",
                            iface.c_str(), stat.ps_recv, stat.ps_drop, pcap_driver_stat.rx_bytes, speed, pcap_driver_stat.rx_packs);
                    fclose(file);
                }
            } else {
                FILE * file = fopen("./libpcap_stat_offline.txt", "w");
                if(file) {
                    fprintf(file, "(LibPcap %s OFFLINE) rx_bytes: %lu(%.2f Mbit/s) rx_packs: %lu, ",
                            iface.c_str(), pcap_driver_stat.rx_bytes, speed, pcap_driver_stat.rx_packs);
                    fclose(file);
                }
            }
            gettimeofday(&prev, nullptr);

            pcap_driver_stat.rx_bytes_prev = pcap_driver_stat.rx_bytes;
            pcap_driver_stat.rx_packs_prev = pcap_driver_stat.rx_packs;
        }
    }

    Driver::DriverStatistic pcap_driver_stat;
private:
    pcap_t* _handle;
    size_t _currPkts;
    Packet** _currBulk;
    bool _offline = false;
    bool _finished = false;

    struct timeval prev;

    std::string iface;
    std::vector<std::string> iface_list;
    size_t index = 0;
};
