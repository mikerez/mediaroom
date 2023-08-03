// Copyright: https://github.com/mikerez/mediaroom/blob/main/LICENSE

#pragma once

#include <boost/interprocess/managed_shared_memory.hpp>
#include <utility>


using namespace boost::interprocess;
class SharedIO
{
public:
    SharedIO() = delete;
    SharedIO(const char * filename, size_t length = 0, bool create = false)
    {
        try {
            if(create) {
                shared_memory_object::remove(filename);
                segment = boost::interprocess::managed_shared_memory(create_only, filename, length);
            }
            else {
                segment = boost::interprocess::managed_shared_memory(open_or_create, filename, length);
            }
        }  catch (std::exception &e) {
            shared_memory_object::remove(filename);
        }

    }

    virtual ~SharedIO()
    {

    }

    SharedIO(const SharedIO &io) = delete;

    SharedIO(SharedIO&& io)
    {
        segment.swap(io.getSegment());
    }

    SharedIO& operator=(SharedIO&& io)
    {
        segment.swap(io.getSegment());
        return *this;
    }

    managed_shared_memory & getSegment()
    {
        return segment;
    }

private:
    managed_shared_memory segment;
};
