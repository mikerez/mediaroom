// Copyright: https://github.com/mikerez/mediaroom/blob/main/LICENSE

#pragma once

#include "stddef.h"
#include <cassert>

template<typename TYPE> class AllocatorList
{
public:
    AllocatorList() throw() {}
    AllocatorList(const AllocatorList&) throw() {}
    template<typename TYPE1>
    AllocatorList(const AllocatorList<TYPE1>&) throw() {}
    ~AllocatorList() throw() {}

    static void init(size_t size)
    {
        _count = _overall = size / sizeof(TYPE);

        _mem = (uint8_t*)malloc(_count * sizeof(TYPE) + 4096);
        if (!_mem) {
            std::string errmsg = "Can't allocate buffer in AllocatorList, size: ";
            errmsg += std::to_string(_count * sizeof(TYPE) + 4096);
            throw std::logic_error(errmsg);
        }
        _array = (Entry*)(_mem + 4096 - (unsigned long long)_array % 4096);

        _begin = _array;
        Entry* entry = _array;
        for (; entry < _array + _count - 1; entry++) {
            entry->next = entry + 1;
        }
        entry->next = nullptr;
        _end = entry;
    }

    static void deinit()
    {
        free(_mem);
    }

    using value_type = TYPE;

//    static size_t size() { return sizeof(TYPE); }
    static size_t count() { return _count; }
    static size_t overall() { return _overall; }

#pragma pack(1)
    struct Entry
    {
        uint8_t data[sizeof(TYPE) - sizeof(Entry*)];
        Entry* next;
    };
#pragma pack()

private:

    static size_t _overall;
    static size_t _count;
    static Entry* _array;
    static uint8_t* _mem;
    static Entry* _begin;
    static Entry* _end;

    static uint8_t* get()
    {
        Entry* entry = _begin;
        if (!entry->next) {
            return nullptr;
        }
        _begin = entry->next;
        --_count;
        return (uint8_t*)entry;
    }

    static void put(uint8_t* ptr)
    {
        Entry* entry = (Entry*)ptr;
        assert(entry >= _array && entry < _array + _overall);
        entry->next = nullptr;
        _end->next = entry;
        _end = entry;
        ++_count;
    }

public:

    template<typename TYPE1>
    struct rebind
    {
        typedef AllocatorList<TYPE1> other;
    };

    static TYPE* allocate(size_t __n, const void* = 0)
    {
        return (TYPE*)get();
    }
    static  void deallocate(TYPE* __p, size_t)
    {
        put((uint8_t*)__p);
    }
    static void construct(TYPE* __p, const TYPE& __val)
    {
        ::new((void*)__p) TYPE(__val);
    }
    static void destroy(TYPE* __p)
    {
        __p->~TYPE();
    }
};

template<> class AllocatorList<void>
{
public:
    template<typename TYPE1>
    struct rebind
    {
        typedef AllocatorList<TYPE1> other;
    };

    using value_type = void;
};

#ifdef _WIN32
// only DEBUG in _WIN32
template<> class AllocatorList<std::_Container_proxy>
{
public:
    AllocatorList() throw() {}
    AllocatorList(const AllocatorList&) throw() {}
    template<typename TYPE1>
    AllocatorList(const AllocatorList<TYPE1>&) throw() {}
    ~AllocatorList() throw() {}

    using value_type = std::_Container_proxy;

public:

    template<typename TYPE1>
    struct rebind
    {
        typedef AllocatorList<TYPE1> other;
    };

    static std::_Container_proxy* allocate(size_t __n, const void* = 0)
    {
        return (std::_Container_proxy*)malloc(sizeof(std::_Container_proxy));
    }
    static  void deallocate(std::_Container_proxy* __p, size_t)
    {
        delete __p;
    }
    static void construct(std::_Container_proxy* __p, const std::_Container_proxy& __val)
    {
        ::new((void*)__p) std::_Container_proxy(__val);
    }
    static void destroy(std::_Container_proxy* __p)
    {
        __p->~_Container_proxy();
    }
};

#endif
