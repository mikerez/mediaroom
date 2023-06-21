// Copyright: https://github.com/mikerez/mediaroom/blob/main/LICENSE

#include <cstdio>
#include <iostream>
#include <memory>
#include <exception>
#include <future>
#include <atomic>
#include <stdlib.h>
#include <time.h>

#include "System.h"
#include "Ip4.h"
#include "Eth.h"
#include "../libshared/SharedServer.h"
#include "PacketPtr.h"
#include "../libshared/SharedMap.h"

struct Mediaroom
{
    struct Config
    {
        short driverBulk = 20;
        static const char *moduleName;
    };
};

const char * Mediaroom::Config::moduleName = "Main";
static const char* const gModuleName = Mediaroom::Config::moduleName;

bool processArgs(int argc, char ** argv, System& system);

bool gExit = false;
// handle all signals
#ifdef _WIN32
BOOL ctrlHandler(DWORD fdwCtrlType)
{
    switch (fdwCtrlType)
    {
    case CTRL_C_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        gExit = true;
        return FALSE;

    default:
        return FALSE;
    }
}
#else
#include <signal.h>
void ctrlHandler(int signum)
{
    gExit = true;
}
#endif

void idle()
{
    static timeval prev = { 0, 0 };
    timeval curr;
    gettimeofday(&curr, nullptr);
    if(TimeHandler::timeval_diff(curr, prev) > 10000000) {
        prev = curr;
        FILE * file = fopen("main_stats.txt", "w");
        if(file)
        {
            fclose(file);
        }
    }
}

int main( int argc, char ** argv )
{
    try {
        System system(argc, argv);  // todo: check errors in this object?

#ifdef WIN32
        if (!SetConsoleCtrlHandler((PHANDLER_ROUTINE)ctrlHandler, TRUE)) {  // a way to stop process
            LOG_ERR(LOG_MAIN, "Cant set ctrl handler\n");
            LOG_EXIT();
            return -1;
        }
#else
        signal(SIGHUP, ctrlHandler);
        signal(SIGPIPE, ctrlHandler);
#endif

        std::vector<Mediaroom::Config> configs;
        system.loadConfig<Mediaroom::Config>(CONFIG_COLUMN_EXT(Mediaroom, driverBulk));
        if (configs.empty()) {
            configs.push_back(Mediaroom::Config{});
        }
        const Mediaroom::Config& config = configs[0];  // later we can select instance by IP or etc

    /*TEST SECTION BEGIN*/
    /*
    Eth<Pkt> eth(nullptr);
    Ip<Eth<Pkt>> ip = eth.move2Ip();
    Tcp<Ip<Eth<Pkt>>> tcp = ip.move2Tcp();
    */
    
    typedef PacketPtr<std::unique_ptr<char[]>> PktBase;
    Eth<PktBase> e(PktBase(std::unique_ptr<char[]>(new char[1024]),1500));
    Ip4<Eth<PktBase>> x(e.makeIp4());
    x->protocol = 1;
    Ip4<Ip4<Eth<PktBase>>> y(x.makeIp4());
//    Ip4<Ip4<Eth<PktBase>>> mmm(move(x));  // same as previous
    y->protocol = 2;
    Ip4<PktBase> z = y.rebase();
    RT_ASSERT(z->protocol == 2);  // test nested cast
    
/*
    Packet* ptr = allocPacket(1500);
    Eth<Pkt> pkt(ptr);
    stack.putEth(move(pkt));
    */

    // Example of using
    uint64_t val;
    SharedMap<uint64_t, uint64_t> sm_write("/dev/shm/shm_map_.shm", 10000, true);
    auto res_ins = sm_write.insert(1, 1);
    res_ins = sm_write.insert(2, 2);
    sm_write.erase(2);
    res_ins = sm_write.insert(3, 3);

    SharedMap<uint64_t, uint64_t> sm_read("/dev/shm/shm_map_.shm");
    auto sm_sec_res = sm_read.find(3, val);
    if(sm_sec_res)
        printf("[RES]: %lu\n", val);
    fflush(stdout);


    srand(time(nullptr));
    size_t lower = 10, highest = 200;
    SharedServer ss;
    auto res = ss.create();
    while(res && !gExit) {
        if(ss.hasClients())
        {
            size_t size = rand() % (highest - lower + 1) + lower;
            uint8_t * data = new uint8_t(size);
            for(auto i = 0; i < size; i++)
            {
                data[i] = rand() % 255;
            }

            LOG_MESS(DEBUG_SYSTEM, "Gen data block with size %li\n", size);

            if(!ss.send(101, data, size))
            {
                LOG_WARN(DEBUG_SYSTEM, "Failed to push to SHMEM!\n");
            }
            delete data;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    }
    catch (const std::exception& ex) {
        LOG_ERR(LOG_MAIN, "Exception: %s\n", ex.what());
    }


    return 0;
}
