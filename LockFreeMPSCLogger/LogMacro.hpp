#pragma once
#include "LockFreeMPSCLogger.hpp"
#include <sstream>

template<typename... Args>
void LogFmt(LOG::LogLevel level, const char* file, int line, const char* func, Args&&... args) {
    std::ostringstream oss;
    (oss << ... << args); 
    std::ostringstream prefix;
    prefix << "[" << file << ":" << line << " " << func << "] ";
    LOG::LockFreeMPSCLogger::instance().log(level, prefix.str() + oss.str());
}

#define LOGS_INFO(...)    LogFmt(LOG::LogLevel::INFO, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOGS_WARNING(...) LogFmt(LOG::LogLevel::WARN, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOGS_ERROR(...)   LogFmt(LOG::LogLevel::ERROR, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOGS_DEBUG(...)   LogFmt(LOG::LogLevel::DEBUG, __FILE__, __LINE__, __func__, __VA_ARGS__)

#define LOG_STREAM(level, stream_expr)\
    do {\
        std::ostringstream oss;\
        oss << stream_expr;\
        LOG::LockFreeMPSCLogger::instance().log(\
            level,\
            std::string("[") + __FILE__ + ":" +\
            std::to_string(__LINE__) + " " + __func__ + "] " + oss.str());\
    } while(0)

#define LOG_INFO(stream_expr)    LOG_STREAM(LOG::LogLevel::INFO, stream_expr)
#define LOG_WARNING(stream_expr) LOG_STREAM(LOG::LogLevel::WARN, stream_expr)
#define LOG_ERROR(stream_expr)   LOG_STREAM(LOG::LogLevel::ERROR, stream_expr)
#define LOG_DEBUG(stream_expr)   LOG_STREAM(LOG::LogLevel::DEBUG, stream_expr)
