#pragma once

#include <thread>
#include <memory>
#include <forward_list>

#include "../../src/core/Debug.h"
#include "../../lib/libshared/SharedFlowAccum.h"
#include "SharedFlowHandler.h"

class SharedFlowDistributor
{
public:
    SharedFlowDistributor(const std::string & shm_name, const int & num_of_handlers);
    ~SharedFlowDistributor();

    void idle();
    void handlePkts();
private:
    static const TimeHandler::usecs_t FLOW_HANDLER_EXIPATION_INTERVAL = 5 * 1000000;
    static const TimeHandler::usecs_t IDLE_INTERVAL = 1 * 1000000;

    TimeHandler::usecs_t _idle_check_ts;
    std::shared_ptr<SharedFlowAccum> _shm_flows;

    std::forward_list<SharedFlowHandler> _flow_handlers;
    std::forward_list<SharedFlowHandler>::iterator _it_curr_flow_handler;

    std::unordered_map<flow_idx_t, std::forward_list<SharedFlowHandler>::iterator> _flow_handlers_map;
    TimeHandler::usecs_t _flow_handlers_map_ckeck_ts;


    void removeOldFlows();
};
