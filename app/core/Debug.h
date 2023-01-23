// Copyright: https://github.com/mikerez/mediaroom/blob/main/LICENSE

#pragma once
#include <stdint.h>
#include <stdio.h>

#include "LogRotate.h"

extern LogRotate g_log;
extern int g_log_level;
extern int g_log_mask;
extern int g_log_param;

#ifndef BUILD_ASSERT
template <bool> struct STATIC_ASSERTION_FAILURE;
template <> struct STATIC_ASSERTION_FAILURE<true> {};
#define BUILD_ASSERT(condition) typedef struct STATIC_ASSERTION_FAILURE<(bool)(condition)> static_assert_fake_type
#define BUILD_ASSERT_INLINE(condition) STATIC_ASSERTION_FAILURE<(bool)(condition)>()
#endif

#define RT_ASSERT(a) { if(!(a)) { printf("\n"); fflush(stdout); *(char*)0=0; } }

#ifndef NDEBUG

#ifdef _WIN32
#define LOG_DEBUG(module, format, ...) if ((1<<module)&g_log_mask) gSystem->logMessage(#module ": " format, __VA_ARGS__)
#define LOG_DEBUG1(module, format, ...) if (g_log_level >= 1 && ((1<<module)&g_log_mask)) { LOG_DEBUG(g_log, #module " " format); }
#define LOG_DEBUG2(module, format, ...) if (g_log_level >= 2 && ((1<<module)&g_log_mask)) { LOG_DEBUG(g_log, #module " " format); }
#else
#define LOG_DEBUG(module, format ...)  if (((1<<module)&g_log_mask)) { LOG_DEBUG(g_log, #module " " format); }
#define LOG_DEBUG1(module, format ...) if (g_log_level >= 1 && ((1<<module)&g_log_mask)) { LOG_DEBUG(g_log, #module ": " format); }
#define LOG_DEBUG2(module, format ...) if (g_log_level >= 2 && ((1<<module)&g_log_mask)) { LOG_DEBUG(g_log, #module ": " format); }
#endif

#else
#define LOG_DEBUG(module, format ...)
#define LOG_DEBUG1(module, format ...)
#define LOG_DEBUG2(module, format ...)
#endif

#ifdef _WIN32

#define DEBUG_EXIT() { exit(-1); }
#define LOG_ERR(module, format, ...) LOG_ERROR(g_log, #module " " format, __VA_ARGS__)
#define LOG_WARN(module, format, ...) LOG_WARNING(g_log, #module " " format, __VA_ARGS__)
#define LOG_MESS(module, format, ...) LOG_MESSAGE(g_log, #module " " format, __VA_ARGS__)
#define LOG_LVL0(module, format, ...) if (((1<<module)&g_log_mask)) LOG_DEBUG(g_log, #module " " format, __VA_ARGS__)
#define LOG_LVL1(module, format, ...) if (g_log_level >= 1 && ((1<<module)&g_log_mask)) LOG_DEBUG(g_log, #module " " format, __VA_ARGS__)
#define LOG_LVL2(module, format, ...) if (g_log_level >= 2 && ((1<<module)&g_log_mask)) LOG_DEBUG(g_log, #module " " format, __VA_ARGS__)
#define LOG_LVL2PARAM(module, param, format, ...) if (g_log_level >= 2 && ((1<<module)&g_log_mask) && ((param) == g_log_param || g_log_param == -1)) LOG_DEBUG(g_log, #module " " format, __VA_ARGS__)

#else

#define DEBUG_EXIT() { exit(-1); }
#define LOG_ERR(module, format ...) LOG_ERROR(g_log, #module " " format)
#define LOG_MESS(module, format ...) LOG_MESSAGE(g_log, #module " " format)
#define LOG_WARN(module, format ...) LOG_WARNING(g_log, #module " " format)
#define LOG_LVL0(module, format ...) if (((1<<module)&g_log_mask)) LOG_DEBUG(g_log, #module " " format)
#define LOG_LVL1(module, format ...) if (g_log_level >= 1 && ((1<<module)&g_log_mask)) LOG_DEBUG(g_log, #module " " format)
#define LOG_LVL2(module, format ...) if (g_log_level >= 2 && ((1<<module)&g_log_mask)) LOG_DEBUG(g_log, #module " " format)
#define LOG_LVL2PARAM(module, param, format ...) if (g_log_level >= 2 && ((1<<module)&g_log_mask) && ((param) == g_log_param || g_log_param == -1)) LOG_DEBUG(g_log, #module " " format)

#endif


class Debug
{
public:
    template<int SIZE>
    struct TextBuffer
    {
        enum {
            Size = SIZE * 2 + 1
        };
        char buffer[(SIZE * 2) + 1];
    };

    template<class BUFFER>
    static const char* printHex(const uint8_t* hex, size_t len, BUFFER&& buf)
    {
        if (len > BUFFER::Size) {
            len = BUFFER::Size;
        }
        int length = 0;
        for (size_t i = 0; i < len; ++i) {
            length += snprintf(buf.buffer + length, BUFFER::Size - length, "%02x", (int) hex[i]);
        }
        return buf.buffer;
    }
};

