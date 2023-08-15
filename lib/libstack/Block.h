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
    bool     is_complete = false;
    uint64_t last_data_ts = 0;

    uint8_t  data[MAX_DATA_SIZE];
    size_t   data_len = 0;

    bool hasSpace(const size_t & size)
    {
        return data_len + size + size > MAX_DATA_SIZE;
    }

    bool expired(const TimeHandler::usecs_t & timestamp)
    {
        if(!last_data_ts) {
            return false;
        }

        return (timestamp - last_data_ts > DEFAULT_BLOCK_EXPIRATION_TIME_MS) ? true : false;
    }

    void push(const uint8_t * new_data, const uint32_t & size)
    {
        memcpy(data + data_len, &size, sizeof (uint32_t));
        data_len += sizeof (uint32_t);
        memcpy(data + data_len, new_data, size);
        data_len += size;
    }

    void reset()
    {
        data_len = 0;
        is_complete = false;
        last_data_ts = 0;
    }

} __attribute__((packed));
#pragma pack()


class BlockL4Wrapper
{
public:
    BlockL4Wrapper() = delete;
    BlockL4Wrapper(Block * block, const bool & create = false, const uint32_t & isn = 0): block(block)
    {
        if(create) {
            memcpy(block->data, &isn, sizeof (uint32_t));
            block->data_len += sizeof (uint32_t);

            initial_seq_number = isn;
            last_seq_number = isn;
        }
        else {
            memcpy(&initial_seq_number, block->data, sizeof (uint32_t));
            last_seq_number = initial_seq_number;
        }
    }

    ~BlockL4Wrapper()
    {
        block = nullptr;
        initial_seq_number = 0;
        last_seq_number = 0;
    }

    Block * block;
    uint32_t initial_seq_number = 0; // Initial flow sequence number (SYN)
    uint32_t last_seq_number    = 0; // Contains last of inserted seq number to block.

    bool push(const uint8_t * new_data, const uint32_t & size, const uint32_t & new_seq)
    {
        /*
         * If there is no missed tcp blocks - try to fill block.
         * Otherwise - need to close block;
         */

        if(block && !block->expired(TimeHandler::Instance()->get_time_usecs()) && block->hasSpace(size)
                 && (new_seq > last_seq_number + 1 || (new_seq == 0 && last_seq_number == -1))) {
            last_seq_number = new_seq;
            block->push(new_data, size);
            return true;
        }
        return false;
    }
private:

};
