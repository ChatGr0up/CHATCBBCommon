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

#define CATCH_DEFAULT \
    catch (const std::invalid_argument& e) { \
        LOG_ERROR("Invalid argument: " << e.what()); \
    } catch (const std::overflow_error& e) { \
        LOG_ERROR("Overflow error: " << e.what()); \
    } catch (const std::bad_alloc& e) { \
        LOG_ERROR("Bad alloc: " << e.what()); \
    } catch (const std::ios_base::failure& e) { \
        LOG_ERROR("IO failure: " << e.what()); \
    } catch (const std::system_error& e) { \
        LOG_ERROR("System error: " << e.what()); \
    }  catch (const std::logic_error& e) { \
        LOG_ERROR("Logic error: " << e.what()); \
    } catch (const std::runtime_error& e) { \
        LOG_ERROR("Runtime error: " << e.what()); \
    } catch (const std::exception& e) { \
        LOG_ERROR("Generic exception: " << e.what()); \
    } catch (...) { \
        LOG_ERROR("Unknown exception"); \
    }

#define CATCH_AND_MSG(msg) \
    catch (const std::invalid_argument& e) { \
        LOG_ERROR(msg << ", Invalid argument: " << e.what()); \
    } catch (const std::overflow_error& e) { \
        LOG_ERROR(msg << ", Overflow error: " << e.what()); \
    } catch (const std::bad_alloc& e) { \
        LOG_ERROR(msg << ", Bad alloc: " << e.what()); \
    } catch (const std::ios_base::failure& e) { \
        LOG_ERROR(msg << ", IO failure: " << e.what()); \
    } catch (const std::system_error& e) { \
        LOG_ERROR(msg << ", System error: " << e.what()); \
    } catch (const std::logic_error& e) { \
        LOG_ERROR(msg << ", Logic error: " << e.what()); \
    } catch (const std::runtime_error& e) { \
        LOG_ERROR(msg << ", Runtime error: " << e.what()); \
    } catch (const std::exception& e) { \
        LOG_ERROR(msg << ", Generic exception: " << e.what()); \
    } catch (...) { \
        LOG_ERROR(msg << ", Unknown exception"); \
    }

#define CATCH_AND_RETURN(ret) \
    catch (const std::invalid_argument& e) { \
        LOG_ERROR("Invalid argument: " << e.what()); \
        return ret; \
    } catch (const std::overflow_error& e) { \
        LOG_ERROR("Overflow error: " << e.what()); \
        return ret; \
    } catch (const std::bad_alloc& e) { \
        LOG_ERROR("Bad alloc: " << e.what()); \
        return ret; \
    } catch (const std::ios_base::failure& e) { \
        LOG_ERROR("IO failure: " << e.what()); \
        return ret; \
    } catch (const std::system_error& e) { \
        LOG_ERROR("System error: " << e.what()); \
        return ret; \
    } catch (const std::logic_error& e) { \
        LOG_ERROR("Logic error: " << e.what()); \
        return ret; \
    } catch (const std::runtime_error& e) { \
        LOG_ERROR("Runtime error: " << e.what()); \
        return ret; \
    } catch (const std::exception& e) { \
        LOG_ERROR("Generic exception: " << e.what()); \
        return ret; \
    } catch (...) { \
        LOG_ERROR("Unknown exception"); \
        return ret; \
    }

#define CATCH_MSG_AND_RETURN(msg, ret) \
    catch (const std::invalid_argument& e) { \
        LOG_ERROR(msg << ", Invalid argument: " << e.what()); \
        return ret; \
    } catch (const std::overflow_error& e) { \
        LOG_ERROR(msg << ", Overflow error: " << e.what()); \
        return ret; \
    } catch (const std::bad_alloc& e) { \
        LOG_ERROR(msg << ", Bad alloc: " << e.what()); \
        return ret; \
    } catch (const std::ios_base::failure& e) { \
        LOG_ERROR(msg << ", IO failure: " << e.what()); \
        return ret; \
    } catch (const std::system_error& e) { \
        LOG_ERROR(msg << ", System error: " << e.what()); \
        return ret; \
    } catch (const std::logic_error& e) { \
        LOG_ERROR(msg << ", Logic error: " << e.what()); \
        return ret; \
    } catch (const std::runtime_error& e) { \
        LOG_ERROR(msg << ", Runtime error: " << e.what()); \
        return ret; \
    } catch (const std::exception& e) { \
        LOG_ERROR(msg << ", Generic exception: " << e.what()); \
        return ret; \
    } catch (...) { \
        LOG_ERROR(msg << ", Unknown exception"); \
        return ret; \
    }
