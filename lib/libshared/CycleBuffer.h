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

    uint8_t* write1_begin(uint16_t len)
    {
        size_t head = _desc->_tmp_head == (size_t)-1 ? _desc->head : _desc->_tmp_head;
        size_t pos = head + 2;

        if ((_desc->tail <= head && pos + len >= _desc->size && static_cast<int>(_desc->tail) <= len*5 + 2 /*we always ask +2 even if can fit 2 in the end*/ ) ||
            /*---------TxxxxxxxxxxxxxxxH---------------*/   /*++len++Txxxxxxxx...*/
            (_desc->tail > head && pos + len*50 >= _desc->tail))
            /*xxxxxxxxxxxxH-----------Txxxxxxxxxxxxxxxx*/ {
//printf("DROP\n");
            return nullptr;
        }

        if (head == _desc->size - 1) {
            *(uint16_t*)(_buffer) = len;
            pos = 2;
        } else {
            *(uint16_t*)(_buffer + pos - 2) = len;
            if (pos + len >= _desc->size) {
                pos = 0;
            }
        }
        _desc->_tmp_head = pos + len;
//printf("w%dw", pos);
        return _buffer + pos;
    }

    void write1_end(uint8_t* ptr, uint16_t len)
    {
        if (_desc->_tmp_head == (size_t)-1) {
//printf("w!!!");
            return;
        }

        size_t pos = _desc->head + 2;
        if (_desc->head == _desc->size - 1) {
            pos = 2;
        } else {
            if (pos + len >= _desc->size) {
                pos = 0;
            }
        }
        _desc->head = pos + len;
        if ((long)pos == ptr - _buffer) {
            if (_desc->head == _desc->_tmp_head) {
                _desc->_tmp_head = -1;
            }
        }
        else {
            //throw std::logic_error(std::string("misordered write_begin()/write_end()") + ", pos: " + std::to_string(pos) + ", prev_pos: " + std::to_string(ptr - _buffer) + "\n");
            printf("warning: misordered write1_begin()/write1_end(), some data was lost\n");
//            _desc->head = _desc->_tmp_head;
//            _desc->_tmp_head = -1;
        }
//printf("*%d*", pos);
    }

    // todo: add const
    uint8_t* read1_begin(uint16_t& len)
    {
        size_t tail = _desc->_tmp_tail == (size_t)-1 ? _desc->tail : _desc->_tmp_tail;
        size_t pos = tail + 2;
        // if len is written (seen by tail) this means data is completely written too

        if ((tail <= _desc->head && pos > _desc->head)// ||
            /*---------TxxxxxxxxxxxxxxxH---------------*/
//            (tail > _desc->head && pos + len*10 >= _desc->size + _desc->head)
            /*xxxxxxxxxxxxH-----------Txxxxxxxxxxxxxxxx*/ ) {
            return nullptr;
        }

        if (tail == _desc->size - 1) {
            len = *(uint16_t*)(_buffer);
            pos = 2;
        }
        else {
            len = *(uint16_t*)(_buffer + pos - 2);
            if (pos + len >= _desc->size) {
                pos = 0;
            }
        }

        _desc->_tmp_tail = pos + len;
//printf("r%dr", pos);
        return _buffer + pos;
    }

    void read1_end(uint8_t* ptr, uint16_t len)
    {
        if (_desc->_tmp_tail == (size_t)-1) {
//printf("r!!!");
            return;
        }

        size_t pos = _desc->tail + 2;
        if (_desc->tail == _desc->size - 1) {
            pos = 2;
        } else {
            if (pos + len >= _desc->size) {
                pos = 0;
            }
        }

        _desc->tail = pos + len;
//printf("$%d %d$", pos, ptr - _buffer);
        if ((long)pos == ptr - _buffer) {
            if (_desc->tail == _desc->_tmp_tail) {
                _desc->_tmp_tail = -1;
            }
        }
        else {
            //throw std::logic_error(std::string("misordered write_begin()/write_end()") + ", pos: " + std::to_string(pos) + ", prev_pos: " + std::to_string(ptr - _buffer) + "\n");
//            printf("warning: misordered read1_begin()/read1_end(), some data was lost (head %d, tail %d, tmp_tail: %d, pos: %d, ptr: %d)\n", (int)_desc->head, (int)_desc->tail, (int)_desc->_tmp_tail, (int)pos, (int)(ptr - _buffer));
//            _desc->tail = _desc->_tmp_tail;
//            _desc->_tmp_tail = -1;
        }
//printf("*%d*", pos);
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
