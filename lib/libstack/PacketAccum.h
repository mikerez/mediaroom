// Copyright: https://github.com/mikerez/mediaroom/blob/main/LICENSE

#pragma once
#include "LibStack.h"

template<class PKT, int BLOCKCNT = 8, class ALLOCATOR = std::allocator<void>, bool REQUIRE_PAGEALIGN = true>
struct PacketAccum
{
public:
    void init()
    {
        BUILD_ASSERT_INLINE(!REQUIRE_PAGEALIGN || sizeof(Block) % 64 == 0);
    }

    bool put(PKT&& pkt)
    {
        if (!_packetsCurrBlock || _packetsCurr == BLOCKCNT-1) {
            Entry* newBlock = (Entry*)Allocator::allocate(1);
            if (newBlock) {
                if (_packetsFirstBlock) {
                    _packetsCurrBlock[BLOCKCNT - 1].next = newBlock;
                    _packetsCurrBlock = newBlock;
                    _packetsCurr = 0;
                }
                else {
                    _packetsFirstBlock = newBlock;
                    _packetsCurrBlock = newBlock;
                }
            }
            else
                return false;
        }
        _packetsCurrBlock[_packetsCurr++].packet = pkt.release();
        _packetsTotal++;
        return true;
    }

    size_t size()
    {
        return _packetsTotal;
    }

#pragma pack(1)
    union Entry
    {
        typename PKT::BasePtr::pointer packet;
        Entry* next;
    };
#pragma pack()

#pragma pack(1)
    typedef struct
    {
        Entry entries[BLOCKCNT];
    } Block;
#pragma pack()

private:
    Entry* _packetsFirstBlock = nullptr;
    Entry* _packetsCurrBlock = nullptr;
    size_t _packetsTotal = 0;
    size_t _packetsCurr = 0;
    typedef typename ALLOCATOR::template rebind<Block>::other Allocator;
};
