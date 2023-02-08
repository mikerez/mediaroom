// Copyright: https://github.com/mikerez/mediaroom/blob/main/LICENSE

#pragma once

#include <stdio.h>
#include <string>
#include <ctime>
#include <future>
#include <thread>
#include <mutex>
#include <stdarg.h>
#include <stdio.h>

#define LOG_CONSOLE
#ifdef LOG_CONSOLE
#define LOG_DEBUG(log, a...)   { fprintf(log, a); fprintf(stdout, a); }
#define LOG_MESSAGE(log, a...) { log.printTime(log); fprintf(log, a); if(LogRotate::flush_logs) fflush(log); log.printTime(stdout); fprintf(stdout, "MESS: " a); if(LogRotate::flush_logs) fflush(stdout); }
#define LOG_WARNING(log, a...) { log.printTime(log); fprintf(log,"WARNING: " a); fflush(log); log.printTime(stdout); fprintf(stdout, "WARNING: " a); fflush(stdout); }
#else
#define LOG_DEBUG(log, a...)   { fprintf(log, a); fprintf(stdout, a); if(LogRotate::flush_logs) fflush(log); }
#define LOG_MESSAGE(log, a...) { log.printTime(log); fprintf(log, a); if(LogRotate::flush_logs) fflush(log); }
#define LOG_WARNING(log, a...) { log.printTime(log); fprintf(log,"WARNING: " a); if(LogRotate::flush_logs) fflush(log); }
#endif
#define LOG_ERROR(log, a...)   { log.printTime(log); fprintf(log,"ERROR: " a);  if(LogRotate::flush_logs) fflush(log); log.printTime(stderr); fprintf(stderr,"ERROR: " a); if(LogRotate::flush_logs) fflush(stderr); }

#define LOG_MAX_FILE_SIZE (1024*1024*1024)
#define LOG_MAX_FILE_TIME (3600)

class LogRotate
{
public:
    LogRotate(const char* fname)
        : filename(fname)
        , activeLog(nullptr)
        , oldLog(nullptr)
        , createTime(std::time(nullptr))
        , check_file_exists_count(0) { }

    ~LogRotate()
    {
        close();
    }

    void setFilename(const std::string file_name)
    {
        this->filename = file_name;
    }

    void setLogPath(const std::string &log_path)
    {
        if(log_path.back() == '/')
            this->m_log_path = log_path.substr(0, log_path.size() - 1);
        else
            this->m_log_path = log_path;
    }

    std::string getLogPath() const { return m_log_path; }

    operator FILE * ()
    {
        check_file_exists_count++;

        if(!activeLog) {
//            fflush(stdout);
            createFile();
        } else {
            if(check_file_exists_count >= check_file_exists_count_limit) {
                FILE *fp = fopen(active_logname.c_str(), "r");
                if(!fp) {
                    // Only if we have not opened old log file
                    if(!oldLog) {
                        fprintf(stderr, "Re-creating log file %s(%i).\n", active_logname.c_str(), errno);
                        oldLog = activeLog;
                        switchTime = std::time(nullptr);
                        createFile();
                    }
                } else {
                    fclose(fp);
                }
                check_file_exists_count = 0;
            }
        }

        return activeLog;
    }

    void createFile()
    {
        active_logname = generateName(std::time(nullptr));
        activeLog = fopen(active_logname.c_str(), "a");
        createTime = std::time(nullptr);
    }

    void check()
    {
        if(!activeLog)
            return;

        auto currTime = std::time(nullptr);
        // Only if we have not opened old log file
        if(!oldLog && ((currTime - createTime) > createNewLogInterval || ftell(activeLog) > LOG_MAX_FILE_SIZE)) {
            std::string new_log_name = generateName(currTime);
            FILE * newLog = fopen(new_log_name.c_str(), "a");
            if(newLog) {
                fflush(activeLog);
                oldLog = activeLog;
                activeLog = newLog;
                createTime = currTime;
                switchTime = currTime;
                active_logname = new_log_name;
            }
        }

        if(oldLog && (currTime - switchTime) > closeOldLogTimeout) {
            fclose(oldLog);
            oldLog = nullptr;
        }
        fflush(activeLog);
    }

    static void printTime(FILE* file)
    {
        auto t = std::time(nullptr);
        tm now;
        localtime_r(&t, &now);
        fprintf(file, "\n%d.%.2d.%.2d %.2d:%.2d:%.2d ", now.tm_year + 1900, (now.tm_mon + 1), now.tm_mday, now.tm_hour, now.tm_min, now.tm_sec);
    }

    void close()
    {
        if(activeLog) {
            fclose(activeLog);
            activeLog = nullptr;
        }
        if(oldLog) {
            fclose(oldLog);
            oldLog = nullptr;
        }
    }

    void periodic_update()
    {
    }

    static const bool flush_logs = false;
    uint64_t log_write_count = 0;
private:
    std::string filename;
    std::string m_log_path = "./";
    FILE * activeLog;
    std::string active_logname;
    FILE * oldLog;

    time_t createTime;
    time_t switchTime;

    int check_file_exists_count;

    const int createNewLogInterval = LOG_MAX_FILE_TIME;
    const int closeOldLogTimeout = 5;
    const int check_file_exists_count_limit = 200;

    std::string generateName(time_t time)
    {
        return std::string(m_log_path + "/" + filename + "-" + std::to_string(static_cast<long>(time)) + ".log");
    }
};
