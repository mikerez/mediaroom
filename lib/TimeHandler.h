// Copyright: https://github.com/mikerez/mediaroom/blob/main/LICENSE

#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <atomic>
#include <string>

#include "Debug.h"

inline unsigned long long rdtsc(void)
{
    unsigned int lo, hi;
    asm volatile ( "rdtsc\n" : "=a" (lo), "=d" (hi) );
    return ((unsigned long long)hi << 32) | lo;
}

class TimeHandler
{
public:
    static TimeHandler * Instance()
    {
        static TimeHandler * time_handler = new TimeHandler();
        return time_handler;
    }

    typedef uint64_t     usecs_t; // µs
    typedef  int64_t    susecs_t; // µs

    static inline timeval usecs_to_timeval(usecs_t usecs)
    {
        return { (time_t)(usecs / USECS_IN_SECS), (suseconds_t)(usecs % USECS_IN_SECS) };
    }
    static inline usecs_t timeval_to_usecs(const timeval & tv) // unsigned
    {
        return tv.tv_sec * USECS_IN_SECS + tv.tv_usec;
    }
    static inline susecs_t timeval_diff(usecs_t usecs_l, usecs_t usecs_r) // signed
    {
        return usecs_l - usecs_r;
    }
    static inline susecs_t timeval_diff(const timeval & tv_l, const timeval & tv_r) // signed
    {
        return timeval_diff(timeval_to_usecs(tv_l), timeval_to_usecs(tv_r));
    }

    inline timeval get_time(uint64_t cpu_ticks = rdtsc()) const
    {
        return usecs_to_timeval(get_time_usecs(cpu_ticks));
    }
    inline usecs_t get_time_usecs(uint64_t cpu_ticks = rdtsc()) const
    {
        return get_time_impl(cpu_ticks);
    }

    uint64_t get_cpu_ticks() const {
        return rdtsc();
    }

    void update_time()
    {
        static usecs_t prev = get_time_usecs();
        usecs_t curr = get_time_usecs();
        if(timeval_diff(curr, prev) < 1000000)
            return;

        auto current_index = index.load();
        const TimeData& td = timedatas[current_index];

        auto tp = set_current_timepoint();
        uint64_t freq = (tp.ticks - td.tp.ticks) * USECS_IN_SECS / (tp.usecs - td.tp.usecs);
        auto update_index = current_index + 1;
        if(update_index >= sizeof(timedatas) / sizeof(timedatas[0]))
            update_index = 0;
        timedatas[update_index] = { tp, (td.freq + freq) / 2 };
        index.store(update_index);

        prev = get_time_usecs();
    }

    static const size_t USECS_IN_SECS = 1000000UL;
private:
    struct TimePoint
    {
        usecs_t  usecs; // µs
        uint64_t ticks;
    };
    struct TimeData
    {
        TimePoint tp;
        uint64_t freq;
    };
    std::atomic<unsigned> index { 0 };
    TimeData timedatas[10];
    uint64_t cpu_freq = 0;

    const std::string FREQ_SCALING_DIR = "/sys/devices/system/cpu/cpu0/cpufreq";
    const std::string CPU_CUR_FREQ_FIL = FREQ_SCALING_DIR + "/scaling_cur_freq";
    const std::string CPU_MIN_FREQ_FIL = FREQ_SCALING_DIR + "/scaling_min_freq";
    const std::string CPU_MAX_FREQ_FIL = FREQ_SCALING_DIR + "/scaling_max_freq";

    TimeHandler()
    {
        if(!get_cpu_freq())
            throw std::logic_error("Can't get CPU frequency");
        if(!check_scaling()) {
            LOG_ERR(LOG_SYSTEM, "\n"
                      "\t !!! WRONG CPU SCALING PARAMETERS !!!\n"
                      "\t !!! TimeHandler may not work correctly !!!\n"
                      "\t !!! SET corrected parameters for CPU scaling !!!\n");
        }
        timedatas[index.load()] = { set_current_timepoint(), cpu_freq };
    }

    bool get_cpu_freq()
    {
        // use CPU_MAX_FREQ_FIL instead of CPU_CUR_FREQ_FIL
        cpu_freq = read_cpu_value(CPU_MAX_FREQ_FIL, 1000);
        printf("CPU freq: %lu\n", cpu_freq);
        return cpu_freq;
    }

    bool check_scaling() const
    {
        return read_cpu_value(CPU_MIN_FREQ_FIL) == read_cpu_value(CPU_MAX_FREQ_FIL);
    }

    TimePoint set_current_timepoint()
    {
        timeval tv;
        uint64_t tks_1 = rdtsc();
        gettimeofday(&tv, nullptr);
        uint64_t tks_2 = rdtsc();
        return { timeval_to_usecs(tv), tks_1/2 + tks_2/2 };
    }

    usecs_t get_time_impl(uint64_t cpu_ticks) const
    {
        TimeData td = timedatas[index.load()];
        int64_t  ticks_div = cpu_ticks - td.tp.ticks;
        susecs_t usecs_div = ticks_div * (int64_t)USECS_IN_SECS / (int64_t)td.freq;
        return td.tp.usecs + usecs_div;
    }

    static size_t read_cpu_value(const std::string& filename, size_t coef = 1)
    {
#ifndef VIRTUAL_MACHINE_MODE
        FILE* file = fopen(filename.c_str(), "r");
        if(!file) {
            LOG_ERR(DEBUG_SYSTEM, "Can't open file '%s' - %s\n", filename.c_str(), strerror(errno));
            return 0;
        }
        char buf[256] = { 0 };
        bool res = fgets(buf, sizeof(buf) - 1, file);
        if(!res) {
            LOG_ERR(DEBUG_SYSTEM, "Can't read file '%s' - %s\n", filename.c_str(), strerror(errno));
        }
        fclose(file);
        size_t val = res ? atol(buf) : 0;
        return val * coef;
#else
        uint64_t cpu_freq;
        FILE* file = fopen("/proc/cpuinfo", "r");
        if(file) {
            const int cpu_freq_line_index = 7;
            int line_index = 0;
            char line[1024];
            while (fgets(line, sizeof(line), file)) {
                if(line_index == cpu_freq_line_index) {
                    int freq_index = 0;
                    while(line[freq_index] != ':')
                        ++freq_index;
                    ++freq_index;
                    while(line[freq_index] == ' ')
                        ++freq_index;

                    auto freq = atof(&line[freq_index]) * 1000000.f;
                    cpu_freq = static_cast<uint64_t>(freq);
                    break;
                }
                ++line_index;
            }
            fclose(file);
            return cpu_freq;
        }
#endif
    }
};
