// Copyright: https://github.com/mikerez/mediaroom/blob/main/LICENSE

#include <cstdio>
#include <iostream>
#include <memory>
#include <exception>
#include <future>
#include <atomic>
#include <stdlib.h>
#include <time.h>
#include <future>

#include "System.h"
#include "Ip4.h"
#include "Eth.h"
#include "../libshared/SharedServer.h"
#include "PacketPtr.h"
#include "../libshared/SharedMap.h"
#include "../libshared/SharedCycleBuffer.h"

#include <boost/crc.hpp>

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
    std::cout << "Using Boost "
              << BOOST_VERSION / 100000     << "."  // major version
              << BOOST_VERSION / 100 % 1000 << "."  // minor version
              << BOOST_VERSION % 100                // patch level
              << std::endl;

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
    

    SharedServer shm_server;
    auto res = shm_server.create();
    srand(time(nullptr));

    size_t lower = 10, highest = 200;
    auto shared_cycle_buff_test_run = [&shm_server, &lower, &highest](std::string shm_cb_name) {
        while(!gExit) {
            if(!shm_server.hasClients()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }
            size_t size = rand() % (highest - lower + 1) + lower;
            uint8_t * data = new uint8_t[size];
            for(auto i = sizeof (uint32_t); i < size; i++) {
                data[i] = rand() % 255;
            }


            // Calc crc to check read errors in app
            boost::crc_32_type crc;
            crc.process_bytes(data + sizeof (uint32_t), size - sizeof (uint32_t));
            uint32_t chechsum = crc.checksum();
            std::memcpy(data, &chechsum, sizeof (uint32_t));

            LOG_MESS(DEBUG_SYSTEM, "[%s]: Gen data block with size %li (CRC: %u)\n", shm_cb_name.c_str(), size, chechsum);

            if(!shm_server.send(101, data, size))
            {
                LOG_WARN(DEBUG_SYSTEM, "Failed to push to SHMEM!\n");
            }

            delete [] data;
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    };

#define TEST_THREADS_NUM    10
    std::vector<std::future<void>> futures;
    for(auto i = 0; i < TEST_THREADS_NUM; i++) {
        futures.push_back(std::async(std::launch::async, shared_cycle_buff_test_run, std::string("cb_" + std::to_string(i))));
    }

    for(auto & future : futures) {
        future.wait();
    }


    /*while(res && !gExit) {
        if(shm_server.hasClients())
        {
            size_t size = rand() % (highest - lower + 1) + lower;
            uint8_t * data = new uint8_t(size);
            for(auto i = 0; i < size; i++)
            {
                data[i] = rand() % 255;
            }

            LOG_MESS(DEBUG_SYSTEM, "Gen data block with size %li\n", size);

            if(!shm_server.send(101, data, size))
            {
                LOG_WARN(DEBUG_SYSTEM, "Failed to push to SHMEM!\n");
            }
            delete data;
        }

    }*/

    }
    catch (const std::exception& ex) {
        LOG_ERR(LOG_MAIN, "Exception: %s\n", ex.what());
    }


    return 0;
}
