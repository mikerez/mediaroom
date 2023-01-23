// Copyright: https://github.com/mikerez/mediaroom/blob/main/LICENSE

#pragma once

#include <string>
#include <vector>
#include <sstream>

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
            char[128] name;
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
            char[1024-128] value;
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
            if (strcmp(_config_list[i].name, name) == 0) {
                _config_list[i].putValue(value);
            }
        }
    }

    struct ConfigParamBase
    {
        ConfigParamBase() {}
        virtual ~ConfigParamBase() {}
        virtual bool putValue(const char* value) = 0;
        std::string name;
    };

    template<class TYPE>
    struct ConfigParam: public ConfigParamBase
    {
        ConfigParam(const char* prefix, const char* param, TYPE* value)
        {
            name = prefix + param;
            parameter = value;
        }
        ~ConfigParam() {}
        TYPE* value;  // must be global var!
        bool putValue(const char* value) {
            std::istringstream(value) >> *value;
        }
    };

    template <typename T>
    void registerParam(T t)
    {
        _config_list.push_back(std::make_unique<T>(t));
    }

    template<typename T, typename... Args>
    void registerParam(T t, Args... args)
    {
        registerParam(args...) ;
    }

    auto getConfigList() {
        return _config_list;
    }

  private:
    std::vector<std::unique_ptr<ConfigParamBase*>> _config_list;

};
