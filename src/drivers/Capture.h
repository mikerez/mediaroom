#pragma once


#include "DriverPcap.h"
#ifdef USE_DPDK
#include "DpdkDriver.h"
#endif
#include "Capture.h"
#include "System.h"
#include "Driver.h"
#include "Debug.h"

enum class DriveType : uint8_t {
    DriverPcap = 1,
    LinPacketDriver = 2,
    SharedIODriver = 3,
    AstartaDriver = 4,
    DpdkDriver = 5,
};

class Capture
{
    struct Config
    {
        std::string iface;
        int mtu = 65535;
        int type = 1; // 1 - pcap, 2 - divert,
                      //        std::shared_ptr<std::string> address;   //  optional
        std::string dpdk_cmd;
        std::string devices;
        static const char *moduleName;
    };
    const char* const gModuleName = Config::moduleName;

public:
    Capture(System& system)
    {
        system.loadConfig<Config>(CONFIG_COLUMN(iface), CONFIG_COLUMN(mtu), CONFIG_COLUMN(type), CONFIG_COLUMN(dpdk_cmd), CONFIG_COLUMN(devices));

        for (Config config : _configs) {
            LOG_MESS(PROBE_CAPTURE, "opening capture iface: %s(%d), mtu: %d\n", config.iface.c_str(), config.type, config.mtu);
            try {
                switch (DriveType(config.type))
                {

                case DriveType::DriverPcap:
                    _drivers.push_back(std::unique_ptr<Driver>(new DriverPcap(config.iface, config.mtu)));
                    break;
                    /*case DriveType::LinPacketDriver:
                    _drivers.push_back(std::unique_ptr<Driver>(new LinPacketDriver(config.iface)));
                    break;
                case DriveType::SharedIODriver:
                    _drivers.push_back(std::unique_ptr<Driver>(new SharedIODriver(config.iface, system)));
                    break;
#ifdef USE_DPDK
                case DriveType::DpdkDriver:
                    printf("Dpdk CMD: %s\n", config.dpdk_cmd.c_str());
                    _drivers.push_back(std::unique_ptr<Driver>(new DpdkDriver("./Probe " + config.dpdk_cmd, config.devices)));
                    break;
#endif*/
                }
            }
            catch (const std::exception& got) {
                LOG_WARN(PROBE_CAPTURE, "ignoring: %s\n", got.what());
            }
        }
        _driversCnt = _drivers.size();
        if (!_driversCnt) {
            LOG_ERR(PROBE_CAPTURE, "No capture devices found\n");
            EXIT();
        }
    }

    ~Capture() {}

    size_t getPackets(Packet** packets, size_t bulkLimit)
    {
        size_t got = 0;
        for ( auto &driverPtr : _drivers ) {
            std::unique_lock<std::mutex> lock(driverPtr.get()->_lock, std::try_to_lock);
            if(lock.owns_lock()) {
                got += driverPtr.get()->getPackets(packets+got, bulkLimit/_driversCnt);
            }
        }
        return got;
    }
    bool finished()
    {
        bool finished = true;
        for (auto &driverPtr : _drivers) {
            std::lock_guard<std::mutex> lock(driverPtr.get()->_lock);
            if (!driverPtr.get()->finished()) finished = false;
        }
        return finished;
    }
    void idle()
    {
        unsigned id = 0;
        for ( auto &driverPtr : _drivers ) {
            std::lock_guard<std::mutex> lock(driverPtr.get()->_lock);
            driverPtr.get()->idle(id++);
        }
    }
private:
    std::vector<Config> _configs;
    std::vector<std::unique_ptr<Driver>> _drivers;
    size_t _driversCnt;
};
