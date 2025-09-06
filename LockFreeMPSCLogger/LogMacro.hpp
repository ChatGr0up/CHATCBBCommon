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

#define LOG_INFO(...)    LogFmt(LOG::LogLevel::INFO, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_WARNING(...) LogFmt(LOG::LogLevel::WARN, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_ERROR(...)   LogFmt(LOG::LogLevel::ERROR, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_DEBUG(...)   LogFmt(LOG::LogLevel::DEBUG, __FILE__, __LINE__, __func__, __VA_ARGS__)
