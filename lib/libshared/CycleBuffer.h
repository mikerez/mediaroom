// Copyright Tribit Ltd, Nizhny Novgorod, tribit.ru

#pragma once
#include <stdint.h>
#include <cstddef>
#include <stdio.h>
#include <exception>
#include <stdexcept>
#include <cstring>

struct CycleBufferDescriptor {
    volatile size_t head;
    volatile size_t _tmp_head;  // needed for async ops
    char cacheLineAlign[64 - sizeof(size_t)*2];
    volatile size_t tail;
    volatile size_t _tmp_tail;  // needed for async ops
    char cacheLineAlign1[64 - sizeof(size_t)*2];
    volatile size_t size;
};

class CycleBuffer {
public:
    CycleBuffer() : _desc(nullptr), _buffer(nullptr), _size(0), _freeMem(false)
    {
    }

    CycleBuffer(uint8_t* buffer, size_t size = 0, bool freeMem = false)
    {
        set(buffer, size, freeMem);
    }

    ~CycleBuffer()
    {
        if (_freeMem) {
            free(_buffer);
        }
    }

    void set(uint8_t * buffer, const size_t & size, const bool & freeMem = false)
    {
        _buffer = buffer;
        _freeMem = freeMem;
        _size = size - sizeof(CycleBufferDescriptor);
        _desc = (CycleBufferDescriptor*)(_buffer + size - sizeof(CycleBufferDescriptor));
        _desc->_tmp_head = -1;  // flush write_begin.write_end
        _desc->_tmp_tail = -1;  // flush read_begin.read_end
    }

    void clear()
    {
        _desc->head = 0;
        _desc->tail = 0;
        _desc->size = _size;
        _desc->_tmp_head = -1;
        _desc->_tmp_tail = -1;
    }

    size_t size() const
    {
        return _desc->size;
    }

    size_t length()
    {
        return _desc->head >= _desc->tail ? _desc->head - _desc->tail : _desc->size - _desc->tail + _desc->head;
    }

    char* print(char* buffer, size_t size)
    {
        snprintf(buffer, size, "head: %u(%d), tail: %u(%d), length: %u", (unsigned)_desc->head, (int) _desc->_tmp_head, (unsigned)_desc->tail, (int) _desc->_tmp_tail, (unsigned)length() );
        return buffer;
    }

    bool write1(const uint8_t* data, uint16_t len)
    {
        size_t pos = _desc->head + 2;

        if ((_desc->tail <= _desc->head && pos + len >= _desc->size && static_cast<int>(_desc->tail) <= len + 2 /*we always ask +2 even if can fit 2 in the end*/) ||
            /*---------TxxxxxxxxxxxxxxxH---------------*/   /*++len++Txxxxxxxx...*/
            (_desc->tail > _desc->head && pos + len >= _desc->tail))
            /*xxxxxxxxxxxxH-----------Txxxxxxxxxxxxxxxx*/ {
            return false;
        }

        if (_desc->head == _desc->size - 1) {
            *(uint16_t*)(_buffer) = len;
            pos = 2;
        } else {
            *(uint16_t*)(_buffer + pos - 2) = len;
            if (pos + len >= _desc->size) {
                pos = 0;
            }
        }
        memcpy(_buffer + pos, data, len);
        _desc->head = pos + len;
        return true;
    }

    const uint8_t * read1(uint16_t & len)
    {
        size_t pos = _desc->tail + 2;
        // if len is written (seen by tail) this means data is completely written too

        if ((_desc->tail <= _desc->head && pos > _desc->head) //||
            /*---------TxxxxxxxxxxxxxxxH---------------*/
//            (_desc->tail > _desc->head && pos > _desc->size + _desc->head)
            /*xxxxxxxxxxxxH-----------Txxxxxxxxxxxxxxxx*/ ) {
            return nullptr;
        }

        if (_desc->tail == _desc->size - 1) {
            len = *(uint16_t*)(_buffer);
            pos = 2;
        }
        else {
            len = *(uint16_t*)(_buffer + pos - 2);
            if (pos + len >= _desc->size) {
                pos = 0;
            }
        }

        _desc->tail = pos + len;
        return _buffer + pos;
    }

    CycleBufferDescriptor* operator ->() {
        return _desc;
    }

private:
    CycleBufferDescriptor* _desc;
    uint8_t * _buffer;
    size_t _size;
    bool _freeMem;
};
