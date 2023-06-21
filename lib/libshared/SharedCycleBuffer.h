// Copyright: https://github.com/mikerez/mediaroom/blob/main/LICENSE

#pragma once

#include "SharedIO.h"
#include "CycleBuffer.h"

class SharedCycleBuffer : public SharedIO
{
public:
    SharedCycleBuffer(const char * filename, size_t length = 0, bool create = false): SharedIO(filename, length, create)
    {
        if (length <= sizeof(CycleBufferDescriptor) + 4096) {
            throw std::logic_error("too small length for SharedIO");
        }

        _cb.set(getMem(), length);
        if (!create) {
            if (length < sizeof(CycleBufferDescriptor) || _cb->size != length - sizeof(CycleBufferDescriptor)) {
                throw std::logic_error("wrong CycleBuffer size when opening: " + std::to_string(_cb.size()) + " != " + std::to_string(length - sizeof(CycleBufferDescriptor)));
            }
        }

        if(create) {
            _cb.clear();
        }
    }

    SharedCycleBuffer(const SharedCycleBuffer & cb) = delete;
    SharedCycleBuffer(SharedCycleBuffer && cb): SharedIO(std::move(cb))
    {
        _cb = cb._cb;
        cb._cb = nullptr;
    }

    SharedCycleBuffer & operator =(SharedCycleBuffer && cb)
    {
        SharedIO::operator=(std::move(cb));
        _cb = cb._cb;
        cb._cb = nullptr;
        return *this;
    }
    CycleBuffer * operator ->()
    {
        return &_cb;
    }
private:
    CycleBuffer _cb;
};
