// Copyright: https://github.com/mikerez/mediaroom/blob/main/LICENSE

#pragma once

#include "SharedIO.h"
#include "CycleBuffer.h"

using namespace boost::interprocess;

class SharedCircularBuffer : public SharedIO
{
public:
    SharedCircularBuffer() = delete;
    SharedCircularBuffer(const std::string & filename, size_t length = 0, bool create = false):
        SharedIO(filename.c_str(), length, create), _mem_name(filename), _mem(nullptr)
    {
        if(create) {
            _mem = getSegment().template construct<uint8_t>(std::string(filename + "_cb").c_str())[length - 1024]();
        }
        else {
            _mem = getSegment().template find<uint8_t>(std::string(filename + "_cb").c_str()).first;
        }

        if(!_mem) {
            std::string err = "Failed to " + std::string(create ? "create" : "find") + " shared object " + filename + "_cb object!";
            throw std::runtime_error(err);
        }

        _cb.set(_mem, length);
        _cb.clear();
    }

    SharedCircularBuffer(const SharedCircularBuffer & cb) = delete;
    SharedCircularBuffer(SharedCircularBuffer && cb): SharedIO(std::move(cb))
    {
    }

    SharedCircularBuffer & operator =(SharedCircularBuffer && cb)
    {
        SharedIO::operator=(std::move(cb));
        _mem = cb._mem;
        cb._mem = nullptr;
        _cb = std::move(cb._cb);
        return *this;
    }

    bool push(const uint8_t * data, const size_t &length)
    {
        if(!_mem) {
            std::string err = "Failed to push data to shared object " + _mem_name + "_cb object!";
            throw std::runtime_error(err);
        }

        return _cb.write1(data, length);
    }

    const uint8_t * pop(uint16_t & length)
    {
        if(!_mem) {
            std::string err = "Failed to pop data to shared object " + _mem_name + "_cb object!";
            throw std::runtime_error(err);
        }

        return _cb.read1(length);
    }

private:
    std::string _mem_name;
    uint8_t * _mem;
    CycleBuffer _cb;
};

