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

#define CONFIG_COLUMN(name) ConfigParser::ConfigParam<std::remove_reference<decltype(&Config::name)>::type>(Config::moduleName, #name, &Config::name)
#define CONFIG_COLUMN_EXT(module, name) ConfigParser::ConfigParam<decltype(&module::Config::name)>(module::Config::moduleName, #name, &module::Config::name)

#if __GLIBC__ == 2 && __GLIBC_MINOR__ < 30
#include <sys/syscall.h>
#define gettid() syscall(SYS_gettid)
#endif

#define EXIT() { exit(-1); }
#define M_VERSION 1.0

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
        else if (argc < 1) {
            return false;
        }
        _config_file = argv[0];
        for (int i = 1; i < argc; i++) {

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
