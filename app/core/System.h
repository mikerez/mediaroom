// Copyright: https://github.com/mikerez/mediaroom/blob/main/LICENSE

#pragma once

#include <vector>
#include <string>
#include <sstream>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <sys/time.h>

#include <map>
#include <unordered_map>

#include "Debug.h"
#include <ConfigParser.h>

#define CONFIG_COLUMN(name) ConfigParser::ConfigParam<decltype(&Config::name)>(Config::moduleName, #name, &Config::name)
#define CONFIG_COLUMN_EXT(module, name) ConfigParser::ConfigParam<decltype(&module::Config::name)>(module::Config::moduleName, #name, &module::Config::name)

#if __GLIBC__ == 2 && __GLIBC_MINOR__ < 30
#include <sys/syscall.h>
#define gettid() syscall(SYS_gettid)
#endif

#define M_VERSION 1.0

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
            LOG_ERR(LOG_SYSTEM, "Can't open file '%s' - %s\n", filename.c_str(), strerror(errno));
            return 0;
        }
        char buf[256] = { 0 };
        bool res = fgets(buf, sizeof(buf) - 1, file);
        if(!res) {
            LOG_ERR(LOG_SYSTEM, "Can't read file '%s' - %s\n", filename.c_str(), strerror(errno));
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

class System
{
    struct Config
    {
        int pcap_collect_mode = 0;
        int log_level = 2;
        int log_mask = 0xFFFFFF;
        int log_param = -1;
        std::string logPath = "./";
        static const char *moduleName;
    };
    const char* const gModuleName = Config::moduleName;

public:
    System(int argc, char** argv)
    {
        processArgs(argc, argv);

        loadConfig<Config>(CONFIG_COLUMN(pcap_collect_mode),
                           CONFIG_COLUMN(log_level),
                           CONFIG_COLUMN(log_mask),
                           CONFIG_COLUMN(log_param),
                           CONFIG_COLUMN(logPath));

        g_log_level = config.log_level;
        g_log_mask = config.log_mask;
        g_log_param = config.log_param;
        LOG_MESS(DEBUG_SYSTEM, "log_level: %d, log_mask: 0x%x, log_param: %d\n", g_log_level, g_log_mask, g_log_param);
    }
    ~System() {}

    static void convert(std::string& dst, const std::string& str)
    {
        dst = str;
    }

    template< typename T >
    static void convert(T& dst, const std::string& str)
    {
        dst = std::stol(str, 0, 0);
    }

/* need this???
    template< typename T >
    static void convert(std::pair<T, T>& dst, const std::string& str)
    {
        // available format: [+-]<digits>-[+-]<digits>
        //  [+-]     - optionally '+' or '-'
        //  <digits> - one or more digit
        // all spaces are ignored
        bool ok = false;
        // delete all spaces
        std::string s = str;
        s.erase(std::remove_if(s.begin(), s.end(),
                               [](char c){ return ::isspace(c); }),
                s.end());
        if (s.size() >= 3) {
            size_t offs = s.find('-', 1);
            if (offs != std::string::npos) {
                convert(dst.first,  str.substr(0, offs));
                convert(dst.second, str.substr(offs + 1));
                ok = true;
            }
        }
        if (!ok) {
            LOG_ERR(LOG_SYSTEM, "wrong pair parameter: %s\n", str.c_str());
        }
    }
*/

    template<class Config, typename... Args>
    void loadConfig(Args... columns)
    {
        _config_parser.registerParam(std::forward<Args>(columns)...);
        auto & config_list = _config_parser.getConfigList();

        // some code to fill from command line
        auto cmdline = _cmdlineConfig.find(Config::moduleName);
        while (cmdline != _cmdlineConfig.end() && cmdline->first == Config::moduleName)
        {
            for (auto & item : config_list)
            {
                auto param = cmdline->second.find(item->name);
                if (item->prefix == Config::moduleName && param != cmdline->second.end()) {
                    item->putValue(param->second.c_str());
                }
            }
            cmdline++;
        }
    }

    bool processArgs(int argc, char** argv)
    {
        if(argc == 2 && strcmp(argv[1], "-v") == 0) {
            printf("Version: %s\n", M_VERSION);
            return false;
        }
        else if (argc < 2) {
            return false;
        }
        _config_file = argv[1];
        for (int i = 2; i < argc; i++) {

            std::string cmd(argv[i]);
            size_t offs;
            if ((offs = cmd.find('{')) != (size_t)-1 && cmd.back() == '}') {

                cmd.pop_back();
                std::string module = cmd.substr(0, offs);
                std::string params = cmd.substr(offs + 1);

                Props props;
                size_t pos = 0;
                size_t index = 0;
                while(pos < params.size()) {
                    std::string name, value;
                    // read name
                    index = params.find(':', index);
                    name = (index != std::string::npos ? params.substr(pos, index - pos) : "");
                    pos = index + 1;
                    // read value
                    //Screened parameter. Do not parse
                    if(params[pos] == '"') {
                        pos++;
                        index = params.find('"', pos);
                        value = params.substr(pos, index - pos);
                        pos = index + 1;
                    } else {
                        index = params.find(',', index);
                        if(index != std::string::npos) {
                            value = params.substr(pos, index - pos);
                            pos = index + 1;
                        } else {
                            value = params.substr(pos);
                            pos += value.size();
                        }
                    }

                    LOG_MESS(LOG_SYSTEM, "setting parameter: %s(%s=%s)\n", module.c_str(), name.c_str(), value.c_str());
                    props.insert({name, value});
                }

                _cmdlineConfig.insert(std::make_pair(module, props));
            }
        }
        return true;
    }

    bool idle()
    {
        //System::init_ts();
        g_log.check();
        return true;
    }

    #define SYSTEM_CLOCK_SEC 1048576
    #define SYSTEM_CLOCK_MS (1048576/1000)
    #define SYSTEM_CLOCK_US (1048576/1000000)
    static uint64_t getClock()  // monotoic time
    {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return (uint64_t)ts.tv_sec*SYSTEM_CLOCK_SEC + ts.tv_nsec/((uint64_t)1000000000/SYSTEM_CLOCK_SEC);
    }

    typedef std::unordered_map<std::string, std::string> Props;
    std::multimap<std::string, Props> _cmdlineConfig;
private:
    std::string _config_file;
    ConfigParser _config_parser;
public:
    Config config;  // seen for all

};
