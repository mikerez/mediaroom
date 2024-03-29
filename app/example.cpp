// Copyright: https://github.com/mikerez/mediaroom/blob/main/LICENSE

#include "../lib/libshared/SharedClient.h"
#include "../src/core/Debug.h"
#include "../lib/libshared/SharedCycleBuffer.h"

#include <unordered_map>
#include <stdlib.h>
#include <time.h>
#include <future>
#include <vector>

#include <boost/crc.hpp>

void handleData(int idx, const uint8_t * data, const size_t & size)
{
    LOG_MESS(DEBUG_APP_EXAMPLE, "[%i]: Read data from SHMEM with size %zu\n", idx, size);

    uint32_t checksum;
    std::memcpy(&checksum, data, sizeof (uint32_t));

    boost::crc_32_type crc;
    crc.process_bytes(data + sizeof (uint32_t), size - sizeof (uint32_t));
    if(crc.checksum() != checksum) {
        LOG_MESS(DEBUG_APP_EXAMPLE, "[%i]: Checksum does not match! In: %u. Calc: %u\n", idx, checksum, crc.checksum());
    }
}

int main(int argc, char *argv[])
{
    shm_id_t id = 0;
    std::string shmem_name;
    ss_cmd cmd;
    SharedClient sc("127.0.0.1", BASE_PORT);

    srand(time(nullptr));
    hash_t hash_upper = 100000000;
    hash_t hash_lower = 99999;
    size_t shmem_size = 10001;

    auto res = sc.create();
    if(!res)
    {
        LOG_ERR(DEBUG_APP_EXAMPLE, "Failed to connenct to Shared Server!\n");
    }

    LOG_MESS(DEBUG_APP_EXAMPLE, "Send INIT ...\n");
    sc.init();
    LOG_MESS(DEBUG_APP_EXAMPLE, "Wait ACK on INIT ...\n");
    sc.waitAck(cmd);
    LOG_MESS(DEBUG_APP_EXAMPLE, "Recv ACK in INIT ...\n");

    LOG_MESS(DEBUG_APP_EXAMPLE, "Send R_CHAN ...\n");
    sc.newRChan("test", shmem_size);
    LOG_MESS(DEBUG_APP_EXAMPLE, "Wait ACK on R_CHAN ...\n");
    sc.waitAck(cmd);
    id = cmd.id;
    shmem_name = std::string(cmd.text);
    LOG_MESS(DEBUG_APP_EXAMPLE, "Recv ACK on R_CHAN %lu:%s ...\n", id, shmem_name.c_str());

    //hash_t hash = rand() % (hash_upper - hash_lower + 1) + hash_lower;
    hash_t hash = 101;
    LOG_MESS(DEBUG_APP_EXAMPLE, "Send BIND on %lu with hash %lu ...\n", id, hash);
    sc.bindChan(id, hash);

    LOG_MESS(DEBUG_APP_EXAMPLE, "Wait ACK on BIND ...\n");
    sc.waitAck(cmd);
    LOG_MESS(DEBUG_APP_EXAMPLE, "Recv ACK in BIND ...\n");

    SharedCircularBuffer * io = new SharedCircularBuffer(shmem_name.c_str(), shmem_size, false);

    auto shared_cycle_buff_test_run = [&io](int idx) {
        while (true) {
            uint16_t data_len = 0;
            auto data = (*io).pop(data_len);
            if(data)
            {
                handleData(idx, data, data_len);
                std::this_thread::sleep_for(std::chrono::milliseconds(30));
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
        }
    };

#define TEST_THREADS_NUM    3
    std::vector<std::future<void>> futures;
    for(auto i = 0; i < TEST_THREADS_NUM; i++) {
        futures.push_back(std::async(std::launch::async, shared_cycle_buff_test_run, i));
    }

    for(auto & future : futures) {
        future.wait();
    }

    res = sc.closeChan(id);
    sc.closeSocket();
    return 0;
}
