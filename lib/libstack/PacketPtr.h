// Copyright: https://github.com/mikerez/mediaroom/blob/main/LICENSE

#pragma once

#include "LibStack.h"
#include <memory>

using std::move;

struct PacketHdr
{
    uint16_t length;
    uint16_t offset;
};
BUILD_ASSERT(sizeof(PacketHdr) == 4);

// smart pointer with ptr->offset tracking
template<class PTR, class PACKET = PacketHdr>
class PacketPtr
{
public:
    PacketPtr()
    {}

    PacketPtr(PacketPtr&& pkt) : 
        _ptr(move(pkt.getPtr()))
    {}

//    PacketPtr(PacketPtr&& pkt, uint16_t length, uint16_t offset = sizeof(Tmp) - sizeof(Tmp::padding)) :
//        _ptr(move(pkt.getPtr()))
//    {
//        setLength(length);
//        setOffset(offset);
//    }

    PacketPtr(PacketPtr& pkt) = delete;

    struct Tmp : public PACKET { uint8_t padding[32]; };  // just to avoid size 1 of empty struct

    PacketPtr(PTR&& ptr, uint16_t length, uint16_t offset = sizeof(Tmp) - sizeof(Tmp::padding)) :
        _ptr(move(ptr))
    {
        setLength(length);
        setOffset(offset);
    }

    PacketPtr(PTR&& ptr) :
        _ptr(move(ptr))
    {}

    PacketPtr& operator=(PacketPtr&& pkt)
    {
        setPtr(move(pkt.getPtr()));
        return *this;
    }

    ~PacketPtr() {}

    uint16_t size() const
    {
        return 0;
    }

    uint16_t fullSize() const
    {
        return 0;
    }

    uint8_t* payload()
    {
        return (uint8_t*)getPtr().get() + getOffset();
    }

    const uint8_t* payload() const
    {
        return (const uint8_t*)getPtr().get() + getOffset();
    }

    uint16_t payloadLength() const
    {
        return getLength();
    }

    typedef PACKET Hdr;
    typedef PacketPtr BaseType;
    typedef PTR BasePtr;

    void setLength(uint16_t length)
    {
        hdr()->length = length;
    }

    uint16_t getLength() const
    {
        return hdr()->length;
    }

    void setOffset(uint16_t offset)
    {
        hdr()->offset = offset;
    }

    uint16_t getOffset() const
    {
        return hdr()->offset;
    }

    BasePtr& getPtr()
    {
        return _ptr;
    }

    const BasePtr& getPtr() const
    {
        return _ptr;
    }

    void setPtr(BasePtr&& ptr)
    {
        _ptr = move(ptr);
    }

    PACKET* hdr()
    {
        return (PACKET*)(get());
    }

    const PACKET* hdr() const
    {
        return (const PACKET*)(get());
    }

    operator PACKET*()
    {
        return (PACKET*)_ptr.get();
    }

    operator const PACKET*() const
    {
        return (const PACKET*)_ptr.get();
    }

    PACKET* release()
    {
        PACKET* prev = _ptr.get();
        _ptr.release();
        return prev;
    }

    void reset(PACKET* ptr = nullptr)
    {
        _ptr.reset(ptr);
    }

    PACKET* get()
    {
        return (PACKET*)_ptr.get();
    }

    const PACKET* get() const
    {
        return (const PACKET*)_ptr.get();
    }

private:
    BasePtr _ptr;
};

// smart pointer with internal offset tracking
template<class PTR, class PACKET = PacketHdr>
class PacketOffsetPtr: public PacketPtr<PTR, PACKET>
{
    typedef PacketPtr<PTR, PACKET> SUB;
public:
    PacketOffsetPtr()
    {}

    PacketOffsetPtr(PacketOffsetPtr&& pkt) : 
        SUB(move(pkt))
    {
        _offset = pkt.getOffset();
    }

    PacketOffsetPtr(SUB&& pkt) : 
        SUB(move(pkt))
    {
        _offset = SUB::getOffset();
    }

    PacketOffsetPtr& operator=(PacketOffsetPtr&& pkt)
    {
        SUB::setPtr(move(pkt.getPtr()));
        setOffset(pkt.getOffset());
        return *this;
    }

    struct Tmp : public PACKET { uint8_t padding[32]; };  // just to avoid size 1 of empty struct
    PacketOffsetPtr(PTR&& ptr, uint16_t length, uint16_t offset = sizeof(Tmp) - sizeof(Tmp::padding))
    {
        SUB::setPtr(move(ptr));
        setOffset(offset);
        // dont change length
    }

    PacketOffsetPtr(PTR&& ptr)
    {
        SUB::setPtr(move(ptr));
        setOffset(SUB::getOffset());
    }

    SUB& operator=(SUB&& pkt)
    {
        setPtr(move(pkt.getPtr()));
        setOffset(SUB::getOffset());
        return *this;
    }

    PacketOffsetPtr(PacketOffsetPtr& pkt) = delete;

    ~PacketOffsetPtr() {}

    void setLength(uint16_t length)
    {
        // dont change length in OffsetPtr
    }

    uint16_t getLength() const
    {
        return SUB::hdr()->length - (_offset - SUB::hdr()->offset);
    }

    void setOffset(uint16_t offset)
    {
        _offset = offset;
    }

    uint16_t getOffset() const
    {
        return _offset;
    }

private:
    uint16_t _offset;
};


template<int level>
struct Depth
{
    auto addDepth() { return Depth<level + 1>(); }
    int getDepth() { return level; }
};

