// Copyright: https://github.com/mikerez/mediaroom/blob/main/LICENSE

#pragma once

#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/range/algorithm/equal_range.hpp>

#include "SharedIO.h"

using namespace boost::interprocess;

template<typename KEY_T, typename VALUE_T>
class SharedMultimap : public SharedIO
{
public:
    typedef std::pair<const KEY_T, VALUE_T> Node;
    typedef allocator<Node, managed_shared_memory::segment_manager> Allocator;
    typedef multimap<KEY_T, VALUE_T, std::less<KEY_T>, Allocator> shmMultimap;

    SharedMultimap() = delete;
    SharedMultimap(const std::string shm_name, const size_t & length = 0, bool create = false):
        SharedIO(shm_name.c_str(), length, create),
        _alloc(getSegment().get_segment_manager())
    {
        if(create) {
            _mmap = offset_ptr<shmMultimap>(getSegment().template construct<shmMultimap>(std::string(shm_name + "_map").c_str())(std::less<KEY_T>(), _alloc));
        }
        else {
            _mmap = offset_ptr<shmMultimap>(getSegment().template find<shmMultimap>(std::string(shm_name + "_map").c_str()).first);
        }
    }
    SharedMultimap(const SharedMultimap & sm) = delete;

    void insert(Node node)
    {
        _mmap->insert(node);
    }

    void erase(const KEY_T & key)
    {
        _mmap->erase(key);
    }

    std::pair<typename shmMultimap::iterator, typename shmMultimap::iterator> find(const KEY_T & key)
    {
        return _mmap->equal_range(key);
    }

private:
    Allocator _alloc;
    offset_ptr<shmMultimap> _mmap;
};
