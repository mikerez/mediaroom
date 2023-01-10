// Copyright: https://github.com/mikerez/mediaroom/blob/main/LICENSE

#pragma once

#include "LibStack.h"
#include "PacketAccum.h"
#include "AllocatorList.h"
#include <map>

#define TCP_ACCUM_ERROR(a...)           { LOG_ERR(LOG_TCP_ACCUM, a); }
#define TCP_ACCUM_DEBUG(a...)           {} //{ LOG_DEBUG(LOG_TCP_ACCUM, a); }
#define TCP_ACCUM_ASSERT(expr, a...)    { if(!(expr)) { TCP_ACCUM_ERROR(a); } }
//#define FULL_CHECK_NODES

template<class PKT, int BLOCKCNT = 10, class ALLOCATOR = std::allocator<int>, bool REQUIRE_PAGEALIGN = false>
class TcpAccum
{
    static const size_t MAX_CACHED_PKTS = 1000;

public:
    ~TcpAccum()
    {
        clean();
    }

    void init(Tcp<PKT>& pkt)  // init by syn
    {
        TCP_ACCUM_ASSERT(!_inited, "Already inited\n");
        _synSeq = LS_ntohl(pkt->sequence) + (pkt->syn ? 1 : 0);
        _lastSeq = _synSeq;
        _inited = true;
        TCP_ACCUM_DEBUG("init(%p): synSeq=%u, lastSeq=%u, inited=%d\n", this, _synSeq, _lastSeq, _inited);
        clean();
    }

    void reset()
    {
        BUILD_ASSERT_INLINE(!REQUIRE_PAGEALIGN || sizeof(typename std::map<uint32_t, Block, std::less<uint32_t>, AllocatorList<std::pair<const uint32_t, Block>>>::value_type) % 64 == 0);
        _inited = false;
        TCP_ACCUM_DEBUG("reset(%p): synSeq=%u, lastSeq=%u, inited=%d\n", this, _synSeq, _lastSeq, _inited);
        clean();
    }

    void clean()
    {
        if (!_pktAccum.empty()) {
            TCP_ACCUM_DEBUG("clean(%p): not empty map\n", this);
        }
        for (auto& item : _pktAccum) {
            for (size_t i = item.second.read; i < item.second.count; ++i) {
                typename PKT::BasePtr::pointer packet = item.second.entries[i].packet;
                Tcp<PKT> pkt = PKT(packet->length, packet, packet->offset);
                item.second.entries[i].packet = 0;
                // pkt deletes itself
            }
        }
        _pktAccum.clear();
        _pktAccumReadIt = _pktAccum.end();
        _countPkts = 0;
    }

    bool inited() const
    {
        return _inited;
    }

    bool is_full() const
    {
        return _countPkts >= MAX_CACHED_PKTS;
    }

    auto find_less(uint32_t seq)
    {
        TCP_ACCUM_DEBUG("find(%p): %sempty map\n", this, _pktAccum.empty() ? "" : "not ");

        auto it = _pktAccum.end();

        if (!_pktAccum.empty()) {
            it = _pktAccum.upper_bound(seq);
            if (it == _pktAccum.begin())  // in TCP zero seq is next to max seq
                it = _pktAccum.end();
            --it;
            TCP_ACCUM_ASSERT(it != _pktAccum.end(), "Logic error!!!\n");
            // check if it->first <= seq
            if (it != _pktAccum.end() && (int32_t)(it->first - seq) > 0)
                it = _pktAccum.end();
        }

        if (it == _pktAccum.end()) {
            TCP_ACCUM_DEBUG("find(%p): not found\n", this);
        }
        else {
            TCP_ACCUM_DEBUG("find(%p): found: first=%u, count=%lu, lastSeq=%u\n",
                            this, it->first, it->second.count, it->second.lastSeq);
            if (!check(it, seq))
                it = _pktAccum.end();
        }

        //assert(it == _pktAccum.end() || it->first <= seq);
        return it;
    }

    Tcp<PKT> put(Tcp<PKT>&& pkt)  // returns 'on-fly' packets
    {
        uint32_t seq = LS_ntohl(pkt->sequence) + (pkt->syn ? 1 : 0);
        if (!_inited) {
            init(pkt);  // init by first packet
        }

        uint32_t newSeq = seq + pkt.payloadLength();
        TCP_ACCUM_DEBUG("put(%p): seq=%u, newSeq=%u, lastSeq=%u\n", this, seq, newSeq, _lastSeq);

        if ((int32_t)(seq - _lastSeq) <= 0) {   // seq <= _lastSeq
            // on-Fly

            if ((int32_t)(newSeq - _lastSeq) <= 0) {    // newSeq <= _lastSeq
                // retransmit
                TCP_ACCUM_DEBUG("put(%p): dup 1\n", this);
                return PKT();  // DUP, free packet
            }
            else {                                      // newSeq > _lastSeq
                // new data
                if ((int32_t)(seq - _lastSeq) < 0) { // seq < _lastSeq
                    // overlaps with old data
                    TCP_ACCUM_DEBUG("put(%p): shift Tcp Seq 1: seq=%u, newSeq=%u, lastSeq=%u, shift=%u\n",
                                    this, seq, newSeq, _lastSeq, _lastSeq - seq);
                    pkt.setPayloadShift(_lastSeq - seq); // set payload offset
                    seq = _lastSeq;
                }

                _lastSeq = newSeq;
                TCP_ACCUM_DEBUG("put(%p): onFly: seq=%u, lastSeq=%u\n", this, seq, _lastSeq);
                return move(pkt);  // return next packet
            }
        }
        else {  // seq > _lastSeq, holes
            auto it = find_less(seq);

            //assert(it == _pktAccum.end() || it->first <= seq);

            if (it != _pktAccum.end()) {
                if ((int32_t)(newSeq - it->second.lastSeq) <= 0) {  // newSeq <= it->second.lastSeq
                    // no new data
                    TCP_ACCUM_DEBUG("put(%p): dup 2\n", this);
                    return PKT();  // DUP, free packet
                }
                else {                                              // newSeq > it->second.lastSeq
                    // new data
                    if ((int32_t)(seq - it->second.lastSeq) < 0) { // seq < it->second.lastSeq
                        // overlaps with old data
                        TCP_ACCUM_DEBUG("put(%p): shift Tcp Seq 2: seq=%u, newSeq=%u, lastSeq=%u, shift=%u\n",
                                        this, seq, newSeq, it->second.lastSeq, it->second.lastSeq - seq);
                        pkt.setPayloadShift(it->second.lastSeq - seq); // set payload offset
                        seq = it->second.lastSeq;
                    }
                }
            }

            if (it != _pktAccum.end() && it->second.count != BLOCKCNT && seq == it->second.lastSeq) {
                TCP_ACCUM_DEBUG("put(%p): found node: first=%u, count=%lu, lastSeq=%u\n",
                                this, it->first, it->second.count, it->second.lastSeq);
            }
            else {
                //                if (Allocator::count())// avoid std::bad_alloc
                it = _pktAccum.emplace_hint(it, std::make_pair(seq, Block()));
                TCP_ACCUM_DEBUG("put(%p): insert node: first=%u, count=%lu, lastSeq=%u\n",
                                this, it->first, it->second.count, it->second.lastSeq);
                if (it->second.count != 0) {
                    TCP_ACCUM_ASSERT(0, "new node count != 0 (%d)", it->second.count);
                    TCP_ACCUM_ERROR("Can't insert node: first=%u, seq=%u, count=%lu\n",
                                    it->first, seq, it->second.count);
                    return PKT();
                }
            }

            it->second.lastSeq = newSeq;
            it->second.entries[it->second.count].seq = seq;
            it->second.entries[it->second.count].packet = pkt.release();
            ++it->second.count;

            TCP_ACCUM_DEBUG("put(%p): insert packet: first=%u, count=%lu, lastSeq=%u\n",
                            this, it->first, it->second.count, it->second.lastSeq);
            TCP_ACCUM_ASSERT(it->second.count <= BLOCKCNT,
                             "Wrong count %lu\n", it->second.count);
            //if(it->second.count > BLOCKCNT)
            //    *(char*)0 = 0;

            ++_countPkts;
            if (is_full()) {
                TCP_ACCUM_DEBUG("put(%p): got MAX countPkts %lu\n", this, _countPkts);
            }
        }

        return PKT();
    }

    Tcp<PKT> get()  // returns cached packets
    {
        TCP_ACCUM_DEBUG("get(%p): find: seq=%u\n", this, _lastSeq);

        // check read iterator
        if (_pktAccumReadIt != _pktAccum.end() &&
            (int32_t)(_pktAccumReadIt->second.entries[_pktAccumReadIt->second.read].seq - _lastSeq) > 0) {

            TCP_ACCUM_ERROR("get: seq=%u, lastSeq=%u\n",
                            _pktAccumReadIt->second.entries[_pktAccumReadIt->second.read].seq, _lastSeq);
            _pktAccumReadIt = _pktAccum.end();
        }

        // create iterator of not set yet
        if (_pktAccumReadIt == _pktAccum.end())
            _pktAccumReadIt = find(_lastSeq);
        else
            check(_pktAccumReadIt, _lastSeq);

        //assert(_pktAccumReadIt == _pktAccum.end() ||
        //       _pktAccumReadIt->second.entries[_pktAccumReadIt->second.read].seq <= _lastSeq);

        // walk thrue all cached packets to find next appropriate
        while (_pktAccumReadIt != _pktAccum.end() &&
               (int32_t)(_pktAccumReadIt->second.entries[_pktAccumReadIt->second.read].seq - _lastSeq) <= 0)
        {
            // take pkt by it
            typename PKT::BasePtr::pointer packet =
                _pktAccumReadIt->second.entries[_pktAccumReadIt->second.read].packet;
            Tcp<PKT> pkt = PKT(packet->length, packet, packet->offset);
            uint32_t seq = _pktAccumReadIt->second.entries[_pktAccumReadIt->second.read].seq;
            uint32_t newSeq = seq + pkt.payloadLength();
            TCP_ACCUM_DEBUG("get(%p): packet: seq=%u, newSeq=%u\n", this, seq, newSeq);
            _pktAccumReadIt->second.entries[_pktAccumReadIt->second.read].packet = 0;

            TCP_ACCUM_ASSERT(_countPkts != 0, "Zero countPkts\n");
            --_countPkts;

            if ((int32_t)(newSeq - _lastSeq) <= 0) {    // newSeq <= _lastSeq
                // no new data
                TCP_ACCUM_DEBUG("get(%p): dup 3\n", this);
                pkt = PKT();    // DUP, free packet
            }
            else {                                      // newSeq > _lastSeq
                // new data
                if ((int32_t)(seq - _lastSeq) < 0) { // seq < _lastSeq
                    // overlaps with old data
                    TCP_ACCUM_DEBUG("get(%p): shift Tcp Seq 3: seq=%u, newSeq=%u, lastSeq=%u, shift=%u\n",
                                    this, seq, newSeq, _lastSeq, _lastSeq - seq);
                    pkt.addPayloadShift(_lastSeq - seq); // shifting payload offset
                    seq = _lastSeq;
                }

                _lastSeq = newSeq;
                TCP_ACCUM_DEBUG("get(%p): found: seq=%u, lastSeq=%u\n", this, seq, _lastSeq);
            }

            ++_pktAccumReadIt->second.read;
            // delete node if all packets were read
            if (_pktAccumReadIt->second.read == _pktAccumReadIt->second.count) {
                TCP_ACCUM_DEBUG("get(%p): erase node: seq=%u\n", this, _pktAccumReadIt->first);
                _pktAccum.erase(_pktAccumReadIt++); // go to next node
                // check if can read next node
                if (_pktAccumReadIt != _pktAccum.end() && (int32_t)(_pktAccumReadIt->first - _lastSeq) > 0)
                    _pktAccumReadIt = _pktAccum.end(); // there is a hole
            }

            if (pkt)
                return move(pkt);
        }

        return PKT();
    }

    template<typename T>
    bool check(const T& it, uint32_t seq) const
    {
#define TCP_ACCUM_CHECK(expr, a...) { if(!(expr)) { TCP_ACCUM_ERROR(a); return false; } }

        TCP_ACCUM_CHECK((int32_t)(it->first - seq) <= 0,   // it->first <= seq
                        "check: first=%u, seq=%u\n", it->first, seq);
        TCP_ACCUM_CHECK(it->second.count > 0 && it->second.count <= BLOCKCNT && it->second.read < it->second.count,
                        "check: count=%lu, read=%lu\n", it->second.count, it->second.read);
        TCP_ACCUM_CHECK(it->first == it->second.entries[0].seq,
                        "check: first=%u, entries[0].seq=%u\n", it->first, it->second.entries[0].seq);
        #ifdef FULL_CHECK_NODES
        auto cloned_pkt = [](Packet* pkt)
        {
            ++pkt->refcnt;
            return Tcp<PKT>(PKT(pkt->length, pkt, pkt->offset));
        };
        const Entry* entry = it->second.entries + it->second.read;
        TCP_ACCUM_CHECK(entry->packet, "check[%lu]: packet=NULL\n", it->second.read);
        uint32_t seq_prev_b = entry->seq;
        uint32_t seq_prev_e = seq_prev_b + cloned_pkt(entry->packet).payloadLength();
        for (size_t i = it->second.read + 1; i < it->second.count; ++i) {
            entry = it->second.entries + i;

            TCP_ACCUM_CHECK(entry->packet, "check[%lu]: packet=NULL\n", i);

            uint32_t seq_curr_b = entry->seq;
            uint32_t seq_curr_e = seq_curr_b + cloned_pkt(entry->packet).payloadLength();
            TCP_ACCUM_CHECK((int32_t)(seq_prev_b - seq_curr_b) < 0 && seq_prev_e == seq_curr_b,
                            "check[%lu]: seq_prev_b=%u, seq_prev_e=%u, seq_curr_b=%u, seq_curr_e=%u\n",
                            i, seq_prev_b, seq_prev_e, seq_curr_b, seq_curr_e);
            seq_prev_b = seq_curr_b;
            seq_prev_e = seq_curr_e;
        }

        TCP_ACCUM_CHECK(seq_prev_e == it->second.lastSeq,
                        "check: seq_prev_b=%u, seq_prev_e=%u, lastSeq=%u\n",
                        seq_prev_b, seq_prev_e, it->second.lastSeq);
        #endif  // FULL_CHECK_NODES

#undef TCP_ACCUM_CHECK

        return true;
    }

#pragma pack(1)
    struct Entry
    {
        typename PKT::BasePtr::pointer packet;
        uint32_t seq;
    }
#ifndef _WIN32
    __attribute__((packed))
#endif
        ;
#pragma pack()

#pragma pack(1)
    typedef struct
    {
        Entry entries[BLOCKCNT];
        size_t count = 0;
        size_t read  = 0;
        uint32_t lastSeq;
        uint32_t unused;
    }
#ifndef _WIN32
    __attribute__((packed))
#endif
        Block;
#pragma pack()

private:
    typedef std::map<uint32_t, Block, std::less<uint32_t>, AllocatorList<std::pair<const uint32_t, Block>>> pktAccumType;
    typedef typename pktAccumType::iterator                                                                 pktAccumTypeIt;

private:
    bool            _inited = false;
    pktAccumType    _pktAccum;
    pktAccumTypeIt  _pktAccumReadIt = _pktAccum.end();
    uint32_t        _synSeq;
    uint32_t        _lastSeq;
    size_t          _countPkts = 0;
};


