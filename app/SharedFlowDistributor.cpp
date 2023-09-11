#include "SharedFlowDistributor.h"

SharedFlowDistributor::SharedFlowDistributor(const std::string &shm_name, const int &num_of_handlers)
{
    _shm_flows = std::make_shared<SharedFlowAccum>(shm_name);
}

SharedFlowDistributor::~SharedFlowDistributor()
{
    _idle_check_ts = TimeHandler::Instance()->get_time_usecs();
    _flow_handlers_map_ckeck_ts = TimeHandler::Instance()->get_time_usecs();

}

void SharedFlowDistributor::idle()
{
    if((TimeHandler::Instance()->get_time_usecs() - _idle_check_ts) < IDLE_INTERVAL) {
        return;
    }


    _idle_check_ts = TimeHandler::Instance()->get_time_usecs();
}

void SharedFlowDistributor::handlePkts()
{
    bool got = true;
    while(got) {
        auto it_block = _shm_flows->get(got);
        if(!got) {
            continue;;
        }

        // Read only start blocks in map.
        // After read of start block - FlowHandler thread will read all related blocks
        if((Block::BlockType)it_block->second.type != Block::BlockType::START_FLOW) {
            continue;
        }

        auto flow_idx = FlowSeqKeyCtx::getFlowIdx(it_block->first);
        auto it_flow_handler = _flow_handlers_map.find(flow_idx);
        if(it_flow_handler == _flow_handlers_map.end()) {
            if(_it_curr_flow_handler == _flow_handlers.end()) {
                _it_curr_flow_handler = _flow_handlers.begin();
            }

            it_flow_handler = _flow_handlers_map.emplace(flow_idx, _it_curr_flow_handler).first;
            _it_curr_flow_handler++;
        }

        it_flow_handler->second->push(it_block);
    }
}

void SharedFlowDistributor::removeOldFlows()
{
    if((TimeHandler::Instance()->get_time_usecs() - _flow_handlers_map_ckeck_ts) < FLOW_HANDLER_EXIPATION_INTERVAL) {
        return;
    }

    for(auto it_flow_handler = _flow_handlers_map.begin(); it_flow_handler != _flow_handlers_map.end();) {
        if(it_flow_handler->second->exired(it_flow_handler->first)) {
            it_flow_handler = _flow_handlers_map.erase(it_flow_handler);
            continue;
        }
        it_flow_handler++;
    }

    _flow_handlers_map_ckeck_ts = TimeHandler::Instance()->get_time_usecs();
}
