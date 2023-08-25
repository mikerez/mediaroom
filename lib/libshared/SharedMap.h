// Copyright: https://github.com/mikerez/mediaroom/blob/main/LICENSE

#pragma once

#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/range/algorithm/equal_range.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>

#include "SharedIO.h"
#include "../TimeHandler.h"

using namespace boost::interprocess;

template<typename KEY_T, typename VALUE_T>
class SharedMultimap : public SharedIO
{

public:
    typedef std::pair<const KEY_T, VALUE_T> Node;
    typedef allocator<Node, managed_shared_memory::segment_manager> Allocator;
    typedef multimap<KEY_T, VALUE_T, std::less<KEY_T>, Allocator> shmMultimap;
    typedef typename shmMultimap::iterator iterator;

    SharedMultimap() = delete;
    SharedMultimap(const std::string shm_name, const size_t & length = 0, bool create = false):
        SharedIO(shm_name.c_str(), length, create),
        _alloc(getSegment().get_segment_manager())
    {
        if(create) {
            _mmap = offset_ptr<shmMultimap>(getSegment().template construct<shmMultimap>(std::string(shm_name + "_map").c_str())(std::less<KEY_T>(), _alloc));
            _shm_mutex = getSegment().template construct<interprocess_mutex>("shm_mutex")();
        }
        else {
            _mmap = offset_ptr<shmMultimap>(getSegment().template find<shmMultimap>(std::string(shm_name + "_map").c_str()).first);
            _shm_mutex = getSegment().template find<interprocess_mutex>("shm_mutex").first;
        }
    }
    SharedMultimap(const SharedMultimap & sm) = delete;

    typename shmMultimap::iterator insert(Node node)
    {
        _shm_mutex->lock();
        auto it = _mmap->insert(node);
        _shm_mutex->unlock();
        return it;
    }

    typename shmMultimap::iterator emplace(KEY_T key, VALUE_T && value)
    {
        _shm_mutex->lock();
        auto it = _mmap->emplace(key, value);
        _shm_mutex->unlock();
        return it;
    }

    void erase(const KEY_T & key)
    {
        _mmap->erase(key);
    }

    std::pair<typename shmMultimap::iterator, typename shmMultimap::iterator> find(const KEY_T & key)
    {
        return _mmap->equal_range(key);
    }

    shmMultimap * getMmap()
    {
        return _mmap.get();
    }

    void lock() {
        _shm_mutex->lock();
    }

    void unlock() {
        _shm_mutex->unlock();
    }
private:
    Allocator _alloc;
    offset_ptr<shmMultimap> _mmap;
    interprocess_mutex * _shm_mutex;

};
