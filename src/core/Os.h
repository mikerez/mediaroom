// Copyright: https://github.com/mikerez/mediaroom/blob/main/LICENSE

#pragma once
#include <stdint.h>

#ifdef _WIN32
#include <WinSock2.h>
#include <Windows.h>
#include <winnt.h>

class Os
{
public:

    typedef struct timeval {
        long tv_sec;
        long tv_usec;
    } timeval;

    static int gettimeofday(struct timeval * tp, struct timezone * tzp)
    {
        // Note: some broken versions only have 8 trailing zero's, the correct epoch has 9 trailing zero's
        // This magic number is the number of 100 nanosecond intervals since January 1, 1601 (UTC)
        // until 00:00:00 January 1, 1970 
        static const uint64_t EPOCH = ((uint64_t)116444736000000000ULL);

        SYSTEMTIME  system_time;
        FILETIME    file_time;
        uint64_t    time;

        GetSystemTime(&system_time);
        SystemTimeToFileTime(&system_time, &file_time);
        time = ((uint64_t)file_time.dwLowDateTime);
        time += ((uint64_t)file_time.dwHighDateTime) << 32;

        tp->tv_sec = (long)((time - EPOCH) / 10000000L);
        tp->tv_usec = (long)(system_time.wMilliseconds * 1000);
        return 0;
    }

    static BOOLEAN bitScanForward (_Out_ unsigned int *Index, _In_ unsigned int Mask)
    {
        return ::_BitScanForward((DWORD*)Index, Mask);
    }

    static BOOLEAN bitScanReverse(_Out_ unsigned int *Index, _In_ unsigned int Mask)
    {
        return ::_BitScanReverse((DWORD*)Index, Mask);
    }
};

typedef Os::timeval timeval;

static inline int gettimeofday(struct timeval *tv, struct timezone *tz)
{
    return Os::gettimeofday(tv, tz);
}

#else
#include <sys/time.h>
#include <strings.h>
#include <string.h>

class Os
{
public:
    static bool bitScanForward (unsigned int *Index, unsigned int Mask)
    {
        *Index = __builtin_ffs(Mask)-1;
        return Mask;
    }

    static bool bitScanReverse(unsigned int *Index, unsigned int Mask)
    {
        *Index = 32 - __builtin_clz(Mask);
        return Mask;
    }
};
#endif
