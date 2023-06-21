// Copyright: https://github.com/mikerez/mediaroom/blob/main/LICENSE

#pragma once

#include <memory>
#include <cassert>
#include <inttypes.h>
#include <unordered_map>
#include <new>

#define VIRTUAL_PAGE_SIZE_4KiB   4096

template<typename TYPE>
class AllocatorList
{
public:
    using value_type = TYPE;
    using size_type = size_t;
    using propagate_on_container_move_assignment = std::true_type;

#pragma pack(1)
    struct Entry
    {
        uint8_t data[sizeof(TYPE) - sizeof(Entry*)];
        Entry * next = nullptr;
    };
#pragma pack()
    struct AllocatorCtx
    {
        Entry * _array;
        Entry * _begin;
        Entry * _end;
        size_t _count;
        size_t _overall;
    };
    typedef struct AllocatorCtx alloc_ctx_t;

    AllocatorList() throw() { };
    AllocatorList(const AllocatorList& a) noexcept: _mem(a._mem) { }
    template<typename TYPE1>
    AllocatorList(const AllocatorList<TYPE1>& other) noexcept: _mem(other.getMem()) { }
    AllocatorList(AllocatorList && a) noexcept: _mem(a._mem) { }
    ~AllocatorList() noexcept { }

    AllocatorList(uint8_t * mem, size_t size, bool create = true): _mem(mem)
    {
        init(mem, size, create);
    }

    const uint8_t * getMem() const
    {
        return _mem;
    }

    const size_t getDataOffset() const
    {
        return sizeof(AllocatorCtx);
    }

    TYPE* allocate(size_t __n, const void* = 0)
    {
        return (TYPE*)get();
    }
    void deallocate(TYPE* __p, size_t)
    {
        put((uint8_t*)__p);
    }
    void construct(TYPE* __p, const TYPE& __val)
    {
        ::new((void*)__p) TYPE(__val);
    }
    void destroy(TYPE* __p)
    {
        __p->~TYPE();
    }

    template<typename TYPE1>
    struct rebind
    {
        typedef AllocatorList<TYPE1> other;
    };

private:
    const uint8_t * _mem;

    uint8_t* get()
    {
        auto alloc_ctx = (alloc_ctx_t *)_mem;
        Entry * entry = alloc_ctx->_begin;
        if (!entry->next) {
            return nullptr;
        }
        alloc_ctx->_begin = entry->next;
        --alloc_ctx->_count;
        return (uint8_t*)entry;
    }

    void put(uint8_t* ptr)
    {
        auto alloc_ctx = (alloc_ctx_t *)_mem;
        Entry * entry = (Entry *)ptr;
        assert(entry >= alloc_ctx->_array && entry < alloc_ctx->_array + alloc_ctx->_overall);
        entry->next = nullptr;
        (alloc_ctx->_end)->next = entry;
        alloc_ctx->_end = entry;
        ++alloc_ctx->_count;
    }

    void init(uint8_t * mem, size_t size, bool create)
    {
        if(create) {
            assert(("Size of shared memory must be grether than virtual page size 4KiB + sizeof(AllocatorCtx)!", size > VIRTUAL_PAGE_SIZE_4KiB + sizeof(AllocatorCtx)));

            alloc_ctx_t * new_ctx = (alloc_ctx_t *)mem;
            new_ctx->_count = new_ctx->_overall = (size - VIRTUAL_PAGE_SIZE_4KiB) / sizeof (TYPE);

            new_ctx->_array = (Entry *)(mem + VIRTUAL_PAGE_SIZE_4KiB - (unsigned long long)new_ctx->_array % VIRTUAL_PAGE_SIZE_4KiB);
            new_ctx->_begin = new_ctx->_array;

            Entry * entry = new_ctx->_array;
            for (; entry < new_ctx->_array + new_ctx->_count - 1; entry++) {
                entry->next = entry + 1;
            }

            entry->next = nullptr;
            new_ctx->_end = entry;
        }
    }
};
