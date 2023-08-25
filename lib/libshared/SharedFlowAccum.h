// Copyright: https://github.com/mikerez/mediaroom/blob/main/LICENSE

#pragma once

#include <unordered_map>
#include <utility>

#include "SharedMap.h"
#include "Debug.h"
#include "../core/Packet.h"
#include "../libstack/Tcp.h"
#include "../libstack/Block.h"

typedef uint64_t flow_seq_key_t;
typedef uint32_t flow_idx_t;
typedef bool     flow_side_t; // false - src, true - dst

inline flow_seq_key_t prepare_key(seq_t seq, flow_idx_t idx, flow_side_t side)
{
    assert(idx < 0x7FFFFFFF);
    return ((flow_seq_key_t)(idx | ((uint32_t)side << 31)) << 32) | seq;
}

struct FlowSeqKeyCtx {
    seq_t seq;
    flow_idx_t idx;
    flow_side_t side;

    FlowSeqKeyCtx(seq_t seq, flow_idx_t idx, flow_side_t side): seq(seq), idx(idx), side(side) { }

    flow_seq_key_t prepare() const
    {
        return prepare_key(seq, idx, side);
    }

    static flow_idx_t getFlowIdx(const flow_seq_key_t & key) {
        return (key >> 32) & 0x7FFFFFFF;
    }

    static flow_side_t getFlowSide(const flow_seq_key_t & key) {
        return (key & 0x80000000) >> 32;
    }

    static seq_t getSeq(const flow_seq_key_t & key) {
        return key & 0xFFFFFFFF;
    }
};

class SharedFlowAccum {
public:
    typedef SharedMultimap<flow_seq_key_t, Block>::iterator shm_mmap_it;

    SharedFlowAccum(const std::string shm_name, const size_t & length = 0, bool create = false);
    ~SharedFlowAccum();

    void put(Tcp<Pkt> && pkt, const flow_idx_t &idx, const flow_side_t &side);
    shm_mmap_it get();
    void removeHandled(shm_mmap_it it_mmap);
    void markExpiredBlocks();
private:
    typedef SharedMultimap<flow_seq_key_t, Block> shm_mmap_t;
    shm_mmap_t _mmap;

    struct ActiveFlowContext {
        shm_mmap_it it_mmap;
        seq_t isn;
        seq_t last_seq;
        TimeHandler::usecs_t update_ts;
    };

    std::unordered_map<flow_idx_t, ActiveFlowContext> _active_flows_ctxs;
    typedef std::unordered_map<flow_idx_t, ActiveFlowContext>::iterator active_flow_ctx_it;

    // Writer
    void create_flow_block(active_flow_ctx_it current_block, const FlowSeqKeyCtx & key_ctx);
    void close_block_and_open_new(active_flow_ctx_it current_block, const FlowSeqKeyCtx & key_ctx);
    active_flow_ctx_it put_start_flow_block(Tcp<Pkt> &pkt, const FlowSeqKeyCtx & key_ctx);
    void push_data_to_block(active_flow_ctx_it it_active_block, Tcp<Pkt> &pkt, const FlowSeqKeyCtx & key_ctx, bool retry = false);
    // Reader
    shm_mmap_it it_read;
};
