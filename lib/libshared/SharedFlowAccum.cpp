#include "SharedFlowAccum.h"

SharedFlowAccum::SharedFlowAccum(const std::string shm_name, const size_t &length, bool create): _mmap(shm_name, length, create)
{

}

SharedFlowAccum::~SharedFlowAccum()
{

}

void SharedFlowAccum::put(Tcp<Pkt> &&pkt, const flow_idx_t & idx, const flow_side_t & side)
{
    uint32_t curr_seq = LS_ntohl(pkt->sequence) + (pkt->syn ? 1 : 0);
    FlowSeqKeyCtx key = { curr_seq, idx, side };

    _mmap.lock();
    auto it_active_block = _active_flows_ctxs.find(idx);
    if(it_active_block == _active_flows_ctxs.end()) {
        // Put to shm mmmap START block. New data block will be created further
        it_active_block = put_start_flow_block(pkt, key);
    }

    // If flow is active, but there is no active block
    if(it_active_block->second.it_mmap == _mmap.getMmap()->end()) {
        auto it_mmap = _mmap.emplace(key.prepare(), Block(Block::BlockType::L4_PAYLOAD));
        it_active_block->second.it_mmap->second = it_mmap->second;
    }

    if(it_active_block->second.it_mmap->second.is_complete || it_active_block->second.it_mmap->second.expired(TimeHandler::Instance()->get_time_usecs())) {
        close_block_and_open_new(it_active_block, key);
    }

    push_data_to_block(it_active_block, pkt, key);

    _mmap.unlock();
}

SharedFlowAccum::shm_mmap_it SharedFlowAccum::get(bool & got)
{
    got = false;
    _mmap.lock();
    if(_mmap.getMmap()->empty()) {
        return _mmap.getMmap()->end();
    }

    auto it_last_read = it_read;
    auto it_block = _mmap.getMmap()->end();
    do {
        if(it_read == _mmap.getMmap()->end()) {
            it_read = _mmap.getMmap()->begin();
        }

        // block must exired after marked is_complete == true
        if(it_read->second.is_complete && it_read->second.expired(TimeHandler::Instance()->get_time_usecs())) {
            it_block = it_read;
            got = true;
        }

        it_read++;
        if(it_block != _mmap.getMmap()->end()) {
            break;
        }
    } while(it_read != it_last_read);

    _mmap.unlock();

    return it_block;
}

void SharedFlowAccum::erase(shm_mmap_it it_mmap)
{
    _mmap.lock();
    _mmap.getMmap()->erase(it_mmap);
    _mmap.unlock();
}

void SharedFlowAccum::markExpiredBlocks()
{
    _mmap.lock();

    auto it_mmap = _mmap.getMmap()->begin();
    while(it_mmap != _mmap.getMmap()->end()) {
        if(!it_mmap->second.is_complete && it_read->second.expired(TimeHandler::Instance()->get_time_usecs())) {
            it_mmap->second.is_complete = true;
            // Finaly block can be handled when time interval after mark close expired.
            it_mmap->second.update_ts();
        }
        it_mmap++;
    }

    _mmap.unlock();
}

SharedFlowAccum::shm_mmap_it SharedFlowAccum::getLowerBoundBlock(flow_seq_key_t key, bool &got)
{
    _mmap.lock();
    got = false;

    if(_mmap.getMmap()->empty()) {
        return _mmap.getMmap()->end();
    }

    auto it_lb_block = _mmap.getMmap()->lower_bound(key);
    if(it_lb_block != _mmap.getMmap()->end()) {
        got = true;
    }

    _mmap.unlock();
    return it_lb_block;
}

SharedFlowAccum::shm_mmap_it SharedFlowAccum::getNextBlock(SharedFlowAccum::shm_mmap_it it_curr_block, bool & got)
{
    _mmap.lock();
    got = true;
    auto it_next_block = _mmap.getMmap()->end();

    if(_mmap.getMmap()->empty()) {
        got = false;
        return _mmap.getMmap()->end();
    }

    if(std::next(it_curr_block) == _mmap.getMmap()->end()) {
        it_next_block = _mmap.getMmap()->begin();
    }
    else {
        it_next_block = std::next(it_curr_block);
    }

    if(it_next_block == it_curr_block) {
        got = false;
    }

    _mmap.unlock();
    return it_next_block;
}

void SharedFlowAccum::create_flow_block(active_flow_ctx_it it_flow_ctx, const FlowSeqKeyCtx & key_ctx)
{
    _mmap.lock();
    auto it_new_block = _mmap.emplace(key_ctx.prepare(), Block(Block::BlockType::L4_PAYLOAD));
    _mmap.unlock();
    it_flow_ctx->second.it_mmap = it_new_block;

    BlockL4Wrapper l4_block_new(&it_flow_ctx->second.it_mmap->second, true, it_flow_ctx->second.isn, key_ctx.seq);
    it_flow_ctx->second.it_mmap->second.update_ts();
    it_flow_ctx->second.last_seq = key_ctx.seq;

}

void SharedFlowAccum::close_block_and_open_new(active_flow_ctx_it it_flow_ctx, const FlowSeqKeyCtx &key_ctx)
{
    // Close current block and put new to mmap
    BlockL4Wrapper l4_block_prev(&it_flow_ctx->second.it_mmap->second);
    it_flow_ctx->second.it_mmap->second.is_complete = true;
    it_flow_ctx->second.it_mmap->second.update_ts();

    // Create new block with new seq key
    create_flow_block(it_flow_ctx, key_ctx);
}


SharedFlowAccum::active_flow_ctx_it SharedFlowAccum::put_start_flow_block(Tcp<Pkt> & pkt, const FlowSeqKeyCtx & key_ctx)
{
    uint32_t curr_seq = LS_ntohl(pkt->sequence) + (pkt->syn ? 1 : 0);
    _mmap.lock();
    auto it_mmap = _mmap.emplace(key_ctx.prepare(), Block(Block::BlockType::L4_PAYLOAD));
    _mmap.unlock();

    // TODO Serialize PacketDetails to data
    it_mmap->second.is_complete = true;
    it_mmap->second.update_ts();

    // Insert to actual flows map, but doesn't create new buffer.
    return _active_flows_ctxs.insert({ key_ctx.idx, { _mmap.getMmap()->end(), curr_seq, curr_seq, TimeHandler::Instance()->get_time_usecs() }}).first;
}

void SharedFlowAccum::push_data_to_block(active_flow_ctx_it it_active_block, Tcp<Pkt> &pkt, const FlowSeqKeyCtx &key_ctx, bool retry)
{
    BlockL4Wrapper l4_block(&it_active_block->second.it_mmap->second);
    auto * data = pkt.data();
    auto data_len = pkt.dataLength();
    switch (l4_block.push(data, data_len, key_ctx.seq)) {
    case BlockL4Wrapper::Code::InconsistentSeq: {
        bool insert_new_block = false;
        // Try to find previous block to push
        auto it_prev_block = it_active_block->second.it_mmap;
        while(true) {
            if(it_prev_block == _mmap.getMmap()->begin()) {
                insert_new_block = true;
                break;
            }
            it_prev_block = std::prev(it_prev_block);

            if(it_prev_block == it_active_block->second.it_mmap) {
                // We checked all previous blocks. Create new block for new pkt.
                insert_new_block = true;
                break;
            }

            if(FlowSeqKeyCtx::getFlowIdx(it_prev_block->first) != key_ctx.idx || FlowSeqKeyCtx::getFlowSide(it_prev_block->first) != key_ctx.side) {
                // Can be sitiation when tcp seq reach max value and need to find pre max tcp seq value
                auto it_prev_max_block = _mmap.getMmap()->lower_bound(FlowSeqKeyCtx((unsigned)-1, key_ctx.idx, key_ctx.side).prepare());
                if(it_prev_max_block != _mmap.getMmap()->end()) {
                    // We found pre max values of tcp seq. Set iterator and try to move backward from this.
                    // No needs to decrease iterator -- now, because it will be performed in cycle.
                    it_prev_block = it_prev_max_block;
                    continue;
                }

                // No more previous blocks for current flow. Insert new block.
                insert_new_block = true;
                break;
            }
            BlockL4Wrapper l4_block_prev(&it_prev_block->second);
            if(key_ctx.seq >= FlowSeqKeyCtx::getSeq(it_prev_block->first) && key_ctx.seq <= l4_block_prev.getLastSeqInBlock()) {
                // This is retransmission - drop
                break;
            }
            // Potential insertion block
            else if(key_ctx.seq >= FlowSeqKeyCtx::getSeq(it_prev_block->first) &&
                    key_ctx.seq > l4_block_prev.getLastSeqInBlock() &&
                    !l4_block_prev.block->is_complete && l4_block_prev.block->hasSpace(data_len)) {
                // Try to append data to block
                if(l4_block_prev.push(data, data_len, key_ctx.seq) != BlockL4Wrapper::Code::Ok) {
                    // Need to open new block.
                    insert_new_block = true;
                    break;
                }
            }

        }

        if(insert_new_block) {
            // Can't find new block -> insert new
            auto it_new_block = _mmap.emplace(key_ctx.prepare(), Block(Block::BlockType::L4_PAYLOAD));
            BlockL4Wrapper l4_block_new(&it_new_block->second, true, it_active_block->second.isn, key_ctx.seq);
            l4_block_new.push(data, data_len, key_ctx.seq);
            it_new_block->second.update_ts();
        }
        break;
    }

    case BlockL4Wrapper::Code::NoBlock:
    case BlockL4Wrapper::Code::NoSpace:
    case BlockL4Wrapper::Code::BlockClosed: {
        // CLose current block if needed and open new and try to write
        close_block_and_open_new(it_active_block, key_ctx);

        if(!retry) {
            push_data_to_block(it_active_block, pkt, key_ctx, true);
        }
        break;
    }
    case BlockL4Wrapper::Code::PushError: {
        LOG_MESS(DEBUG_TCP_ACCUM, "Unhandled case during push data to block!\n");
        break;
    }
    default: break;
    }
}
