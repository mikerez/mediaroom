// Copyright: https://github.com/mikerez/mediaroom/blob/main/LICENSE

#pragma once

#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <string>
#include <stdexcept>

#ifdef WIN32
#include <io.h>
#include <conio.h>
#include <windows.h>
#include <tchar.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

class SharedIO
{

public:
    SharedIO() : _fd(-1), _length(0), _memory(nullptr)
    {
    }
    SharedIO(const char* filename, size_t length = 0, bool create = false)
    {
#ifdef WIN32
        errno_t err =_sopen_s(&_fd, "text.txt", _O_RDWR | _O_CREAT, _SH_DENYNO,
            _S_IREAD | _S_IWRITE);/*{
            _tprintf(TEXT("1Could not create file mapping object (%d).\n"),
                GetLastError());*/
        //printf("%d %d\n", err, _fd);
        /*if ((int)(hFile = (HANDLE)OpenFile("text.txt",
            &of,
            OF_READ)) == -1){
            _tprintf(TEXT("1Could not create file mapping object (%d).\n"),
                GetLastError());
        }*/
#else
        _fd = open(filename, (create?O_CREAT:0)|O_RDWR, S_IRUSR|S_IWUSR);
        if (_fd < 0) {
            throw std::logic_error(std::string("Can't open file: ") + filename);
        }
#endif
        _length = length;

        struct stat buf;
        if (fstat(_fd, &buf) >= 0 && buf.st_size > 0) {
            if ((long)_length != buf.st_size && _length != 0) {  // doesn't look like shmem
                throw std::logic_error("wrong length of SharedIO");
            }
            _length = buf.st_size;
            create = false;  // don't init pointers
        }
        else {
            if (create) {
                truncate();
            }
        }

#ifdef WIN32
        if (!(hMMFile = CreateFileMapping(INVALID_HANDLE_VALUE,
            NULL,
            PAGE_READWRITE,
            0,
            _length*2,
            TEXT("Local\\test"))
            )) {
            _tprintf(TEXT("2Could not create file mapping object (%d).\n"),
                GetLastError());
        }

        if (!(_memory = MapViewOfFile(hMMFile,
            FILE_MAP_ALL_ACCESS,
            0,
            0,
            _length*2))) {
            _tprintf(TEXT("3Could not create file mapping object1 (%d).\n"),
                GetLastError());
        }
#else
        if ((_memory = mmap(0, _length, PROT_WRITE|PROT_READ, MAP_SHARED, _fd, 0)) == MAP_FAILED) {
            throw std::logic_error(std::string("Can't mmap file: ") + std::to_string(_length));
        }
        if (mlock(_memory, _length)) {  // prevent syncing to disk
            /*throw std::logic_error*/ printf("mlock failed\n");
        }
#endif
    }

    virtual ~SharedIO()
    {
        if (_fd != -1) {
#ifdef WIN32
            UnmapViewOfFile(_memory);
            CloseHandle(hMMFile);
            
#else
            munmap(_memory, _length);
            close(_fd);

#endif
        }
    }

    SharedIO(const SharedIO &io) = delete;

    SharedIO(SharedIO&& io)
    {
        _fd = io._fd;
        io._fd = 0;
        _length = io._length;
        io._length = 0;
        _memory = io._memory;
        io._memory = nullptr;
    }

    SharedIO& operator=(SharedIO&& io)
    {
        _fd = io._fd;
        io._fd = 0;
        _length = io._length;
        io._length = 0;
        _memory = io._memory;
        io._memory = nullptr;
        return *this;
    }

    bool truncate()
    {
#ifdef WIN32
        return _chsize(_fd, _length);
#else
        return ftruncate(_fd, _length) < 0;
#endif
    }

    uint8_t* getMem()
    {
        return (uint8_t*)_memory;
    }
private:
#ifdef WIN32
    HANDLE  hFile;
    HANDLE hMMFile;
#endif
    int _fd;
    size_t _length;
    void* _memory;
};
