// Copyright: https://github.com/mikerez/mediaroom/blob/main/LICENSE

#pragma once

#include <stdint.h>

#ifndef BUILD_ASSERT
template <bool> struct STATIC_ASSERTION_FAILURE;
template <> struct STATIC_ASSERTION_FAILURE<true> {};
#define BUILD_ASSERT(condition) typedef struct STATIC_ASSERTION_FAILURE<(bool)(condition)> static_assert_fake_type
#define BUILD_ASSERT_INLINE(condition) STATIC_ASSERTION_FAILURE<(bool)(condition)>()
#endif

#ifndef LS_ASSERT
#ifndef NDEBUG
#define LS_ASSERT(a) { if (!(a)) *(char*)0 = 0; }
#else
#define LS_ASSERT(a) {  }
#endif
#endif

#ifdef _WIN32
#include <WinSock2.h>
inline uint16_t LS_ntohs(uint16_t val)
{
    return _byteswap_ushort(val);
}

inline uint32_t LS_ntohl(uint32_t val)
{
    return _byteswap_ulong(val);
}
#else
#include <arpa/inet.h>
inline uint16_t LS_ntohs(uint16_t val)
{
    return ntohs(val);
}

inline uint32_t LS_ntohl(uint32_t val)
{
    return ntohl(val);
}
#endif

