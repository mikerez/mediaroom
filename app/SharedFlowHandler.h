#pragma once

#include <unordered_map>
#include <thread>
#include <mutex>

#include "../../lib/libshared/SharedFlowAccum.h"

static bool gWork = true;

class SharedFlowHandler {
public:
    SharedFlowHandler(std::shared_ptr<SharedFlowAccum> shm_accum_map);
    ~SharedFlowHandler();
    void run();
    void push(SharedFlowAccum::shm_mmap_it it_block);
    bool exired(const flow_idx_t & idx) const;
    void remove(const flow_idx_t & idx);
private:
    static const TimeHandler::usecs_t FLOW_EXPIRATION_INVERVAL = 10 * 1000000;

    struct FlowContext {
        FlowContext(const seq_t & isn, SharedFlowAccum::shm_mmap_it it_block): isn(isn), last_seq(isn) {
            update_time = TimeHandler::Instance()->get_time_usecs();
        }

        void updateLastSeq(const seq_t & new_seq) {
            last_seq = new_seq;
            update_time = TimeHandler::Instance()->get_time_usecs();
        }

        seq_t isn;
        seq_t last_seq;
        TimeHandler::usecs_t update_time;
    };

    std::shared_ptr<SharedFlowAccum> _shm_accum_map;
    typedef std::unordered_map<flow_idx_t, FlowContext> flow_ctx_map_t;
    flow_ctx_map_t _flow_ctx_map;
    flow_ctx_map_t::iterator _it_flow_ctx_map_current;
    std::mutex _flow_ctx_map_lock;
    std::thread _thread;

};
