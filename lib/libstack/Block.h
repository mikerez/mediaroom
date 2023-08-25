// Copyright: https://github.com/mikerez/mediaroom/blob/main/LICENSE

#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../TimeHandler.h"

/*
 * For L4_PAYLOAD: every message has TCP/UDP hdr. In the beginning of data there is initial sequence number from TCP.
 * For L7_PAYLOAD: transfer only payload
 *
 * START_FLOW: Notify app of start of new flow - send once all headers before (Eth, IPv4/6)
 * END_FLOW: Notify app of end of flow
 */

typedef uint32_t seq_t;

#pragma pack(1)
struct Block {
    static const size_t DEFAULT_BLOCK_CNT = 32;
    static const size_t DEFAULT_TCP_BLOCK_SIZE = 256;
    static const size_t MAX_DATA_SIZE = DEFAULT_TCP_BLOCK_SIZE * DEFAULT_BLOCK_CNT;
    static const size_t DEFAULT_BLOCK_EXPIRATION_TIME_MS = 100;

    enum class BlockType {
        UNKNOWN,
        ADDR,
        L3_PAYLOAD,
        L4_PAYLOAD,
        L7_PAYLOAD,

        START_FLOW,
        END_FLOW,
    };

    Block(const BlockType & block_type)
    {
        type = (uint32_t)block_type;
    }

    uint32_t type;
    uint32_t block_size = 0;
    volatile bool     is_complete = false;
    volatile TimeHandler::usecs_t update_timestamp = 0;

    uint8_t  data[MAX_DATA_SIZE];
    size_t   data_len = 0;

    void update_ts()
    {
        update_timestamp = TimeHandler::Instance()->get_time_usecs();
    }

    bool hasSpace(const size_t & size)
    {
        return data_len + size + size > MAX_DATA_SIZE;
    }

    bool expired(const TimeHandler::usecs_t & timestamp = TimeHandler::Instance()->get_time_usecs())
    {
        if(!update_timestamp) {
            return false;
        }

        return (timestamp - update_timestamp > DEFAULT_BLOCK_EXPIRATION_TIME_MS) ? true : false;
    }

    void push(const uint8_t * new_data, const uint32_t & size)
    {
        memcpy(data + data_len, &size, sizeof (uint32_t));
        data_len += sizeof (uint32_t);
        memcpy(data + data_len, new_data, size);
        data_len += size;
        update_ts();
    }

    void reset()
    {
        data_len = 0;
        is_complete = false;
        update_timestamp = 0;
    }

    inline auto empty() const
    {
        return data_len == 0;
    }

} __attribute__((packed));
#pragma pack()


class BlockL4Wrapper
{
public:
    enum class Code {
        Ok,
        NoBlock,
        NoSpace,
        InconsistentSeq,
        BlockClosed,
        PushError
    };

    BlockL4Wrapper() = delete;
    BlockL4Wrapper(Block * block, const uint32_t & curr_seq = 0, const bool & create = false, const uint32_t & isn = 0): block(block)
    {
        hdr = (L4Hdr *)block;
        if(create) {
            hdr->initial_seq_number = isn;
            hdr->last_seq_number_in_block = 0;
        }
    }

    ~BlockL4Wrapper()
    {
        hdr = nullptr;
        block = nullptr;
    }

    seq_t get_isn() const
    {
        return hdr->initial_seq_number;
    }

    seq_t getLastSeqInBlock() const {
        return hdr->last_seq_number_in_block;
    }

    Code push(const uint8_t * new_data, const uint32_t & size, const uint32_t & new_seq)
    {
        /*
         * If there is no missed tcp blocks - try to fill block.
         * Otherwise - need to close block;
         */

        if(!block) {
            return Code::NoBlock;
        }
        else if(block->is_complete) {
            return Code::BlockClosed;
        }
        else if(!block->hasSpace(size)) {
            // Close block instantly if there is no space
            block->is_complete = true;
            block->update_ts();
            return Code::NoSpace;
        }
        else if(block->empty() || (new_seq > hdr->last_seq_number_in_block + 1) || (new_seq == 0 && hdr->last_seq_number_in_block == -1)) {
            block->push(new_data, size);
            hdr->last_seq_number_in_block = new_seq;
            return Code::Ok;
        }

        return Code::PushError;
    }

public:
    Block * block;
private:
    struct L4Hdr {
        seq_t initial_seq_number;
        seq_t last_seq_number_in_block;
    };

    L4Hdr * hdr;
};
