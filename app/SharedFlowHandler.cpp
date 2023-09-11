#include "SharedFlowHandler.h"
#include "Block.h"

#include <condition_variable>

SharedFlowHandler::SharedFlowHandler(std::shared_ptr<SharedFlowAccum> shm_accum_map): _shm_accum_map(shm_accum_map)
{
    _thread = std::thread(&SharedFlowHandler::run);
    _thread.detach();
}

SharedFlowHandler::~SharedFlowHandler()
{

}

void SharedFlowHandler::run()
{
    while(gWork) {
        auto it_flow_ctx = _flow_ctx_map.begin();
        while(it_flow_ctx != _flow_ctx_map.end()) {
            bool got;
            auto it_flow_block = _shm_accum_map->getLowerBoundBlock(((uint64_t)it_flow_ctx->first) << 32, got);
            while(got) {
                // TODO handle block
                // Insert Session.h from this

                // Remove handled block
                _shm_accum_map->erase(it_flow_block);
                it_flow_block = _shm_accum_map->getNextBlock(it_flow_block, got);
            }

            it_flow_ctx++;
        }
    }
}

void SharedFlowHandler::push(SharedFlowAccum::shm_mmap_it it_block)
{
    std::lock_guard<std::mutex> lock(_flow_ctx_map_lock);
    auto flow_idx = FlowSeqKeyCtx::getFlowIdx(it_block->first);
    auto it_flow = _flow_ctx_map.find(flow_idx);
    if(it_flow != _flow_ctx_map.end()) {
        return;
    }

    BlockL4Wrapper block_l4(&it_block->second);
    _flow_ctx_map.emplace(flow_idx, FlowContext(block_l4.get_isn(), it_block));
}

bool SharedFlowHandler::exired(const flow_idx_t &idx) const
{
    auto it_flow = _flow_ctx_map.find(idx);
    if(it_flow != _flow_ctx_map.end()) {
        return true;
    }

    return (TimeHandler::Instance()->get_time_usecs() - it_flow->second.update_time) > FLOW_EXPIRATION_INVERVAL;
}

void SharedFlowHandler::remove(const flow_idx_t &idx)
{
    std::lock_guard<std::mutex> lock(_flow_ctx_map_lock);
    _flow_ctx_map.erase(idx);
}
