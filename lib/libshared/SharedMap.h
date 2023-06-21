// Copyright: https://github.com/mikerez/mediaroom/blob/main/LICENSE

#pragma once

#include "SharedIO.h"
#include "AllocatorList.h"

#include <map>
#include <mutex>

template<typename KEY_T, typename VALUE_T>
class SharedMap : public SharedIO
{
public:
    typedef AllocatorList<std::_Rb_tree_node<std::pair<const KEY_T, VALUE_T>>> alloc_t;
    typedef std::map<KEY_T, VALUE_T, std::less<KEY_T>,alloc_t>                 shmMap;
    typedef typename shmMap::iterator                                          shmMapIt;

    SharedMap(const char * filename, size_t length = 0, bool create = false):
        SharedIO(filename, length, create),
        _alloc(alloc_t(getMem() + _shared_mem_offset, length - _shared_mem_offset, create))
    {

        size_t offset = 0;
        if(create) {
            _lock = new (getMem() + offset) std::mutex();
            offset += sizeof (std::mutex);
            _map = new (getMem() + offset) shmMap(_alloc);
            offset += sizeof (shmMap);
        }
        else {
            _lock = (std::mutex *)(getMem() + offset);
            offset += sizeof (std::mutex);
            _map = (shmMap *)(getMem() + offset);
            offset += sizeof (shmMap);
        }
    }

    SharedMap(const SharedMap & sm) = delete;
    SharedMap(SharedMap && sm): SharedIO(std::move(sm)) { }
    ~SharedMap()
    {
    }

    bool find(KEY_T key, VALUE_T & val)
    {
        std::lock_guard<std::mutex> lock(*_lock);
        auto it_res = _map->find(key);
        if(it_res != _map->end())
        {
            val = it_res->second;
        }
        return it_res != _map->end() ? true : false;
    }

    bool insert(KEY_T key, VALUE_T value)
    {
        std::lock_guard<std::mutex> lock(*_lock);
        return _map->insert({ key, value }).second;
    }

    void erase(KEY_T key)
    {
        std::lock_guard<std::mutex> lock(*_lock);
        _map->erase(key);
    }
private:
    const size_t _shared_mem_offset = sizeof (std::mutex) + sizeof (shmMap);
    alloc_t _alloc;
    std::mutex * _lock;
    shmMap * _map;
};
