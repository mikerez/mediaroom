// Copyright: https://github.com/mikerez/mediaroom/blob/main/LICENSE

#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <memory>
#include <string.h>
#include "Debug.h"

class ConfigParser
{
  public:
    ConfigParser() {}
    ~ConfigParser() {}

    void loadConfigFromFile(const std::string& filename)
    {
        FILE* file = fopen(filename.c_str(), "r");
        if (!file) {
            throw std::logic_error(std::string("Can't open file: ") + filename);
        }
        char line[1024];
        while (fgets(line, sizeof(line), file)) {
            char* ptr = line;
            while (*ptr == ' ' || *ptr == '\t') {
                ++ptr;
            }
            if (*ptr == '#') {
                continue;
            }
            char name[128];
            sscanf(ptr, "%127s", name);
            while (*ptr == ' ' || *ptr == '\t') {
                ++ptr;
            }
            if (*ptr != '=') {
                continue;
            }
            ++ptr;
            while (*ptr == ' ' || *ptr == '\t') {
                ++ptr;
            }
            bool quota = false, dquota = false;
            if (*ptr == '\'') {
                ++ptr;
                quota = true;
            }
            if (*ptr == '\"') {
                ++ptr;
                dquota = true;
            }
            char value[1024-128];
            unsigned size = 0;
            bool backslash = false;
            while (*ptr && !(*ptr == '\'' && quota) && (!(*ptr == '\"' && dquota) || backslash) && size < 1024-128-1) {
                if (dquota && *ptr == '\\') {
                    backslash = true;
                    continue;
                }
                if (!quota && dquota && *ptr == ' ') {
                    break;
                }
                value[size++] = *ptr;
                backslash = false;
            }
            value[size] = 0;
            //
            putConfig(name, value);
        }
    }

    void putConfig(const char* name, const char* value)
    {
        for (int i=0; i<_config_list.size(); ++i) {
            if (strcmp(_config_list[i]->name.c_str(), name) == 0) {
                _config_list[i]->putValue(value);
            }
        }
    }

    struct ConfigParamBase
    {
        ConfigParamBase() {}
        virtual ~ConfigParamBase() {}
        virtual bool putValue(const char* value) = 0;

        std::string prefix;
        std::string name;
    };

    template<typename  T>
    struct ConfigParam: public ConfigParamBase
    {
        ConfigParam(const char* prefix, const char* param, T value)
        {
            this->prefix = prefix;
            this->name = param;
            this->value = value;
        }
        ~ConfigParam() {}
        bool putValue(const char* new_value) {
            std::stringstream stream;
            stream << new_value;
            stream >> value;
        }

        T value;  // must be global var!
    };

    void registerParam() {}

    template <typename Head, typename ...Tail>
    void registerParam(Head & h, Tail &... t) {
        _config_list.push_back(std::make_unique<Head>(h));
        registerParam(t...);
    }

    template<typename... Args>
    void registerParam(Args... args)
    {
        registerParam(args...);
    }

    auto & getConfigList() {
        return _config_list;
    }

  private:
    std::vector<std::unique_ptr<ConfigParamBase>> _config_list;

};
