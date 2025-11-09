#pragma once
#include <atomic>
#include <thread>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <chrono>
#include <ctime>
#include <condition_variable>
#include <mutex>
#include <memory>
#include <sys/uio.h>  
#include <unistd.h>   
#include <fcntl.h>   
#include <filesystem>
#include "boost/lockfree/queue.hpp"

namespace LOG {
enum class LogLevel { DEBUG, INFO, WARN, ERROR };

struct LogItem {
    LogLevel level;
    std::string msg;
    std::string timestamp;
    std::size_t producer_tid{0};
};

class LockFreeMPSCLogger {
public:
    static LockFreeMPSCLogger& instance() {
        static LockFreeMPSCLogger log;
        return log;
    }

    void log(LogLevel level, std::string msg) {
        gettime(time_buf, sizeof(time_buf));
        auto item = item_pool_->acquire();
        item->level = level;
        item->msg = std::move(msg);
        item->timestamp = time_buf;
        item->producer_tid = getpid();
        unsigned spin = 0;
        while (!queue_.push(item)) {
            if (spin < 64) {
                ++spin;
                std::this_thread::yield();
            } else {
                std::this_thread::sleep_for(std::chrono::microseconds(50));
                spin = 0;
            }
        }
    }

private:
    LockFreeMPSCLogger() : queue_(1024) { start(); };
    ~LockFreeMPSCLogger() { stop(); }

    LockFreeMPSCLogger(const LockFreeMPSCLogger&) = delete;
    LockFreeMPSCLogger& operator=(const LockFreeMPSCLogger&) = delete;

    void start(size_t queue_size = 1024) {
        item_pool_ = std::make_unique<LogItemPool>(queue_size);
        running_.store(true, std::memory_order_release);
        consumer_thread_ = std::thread(&LockFreeMPSCLogger::consumeLoop, this);
    }

    void stop() {
        running_.store(false, std::memory_order_release);
        if (consumer_thread_.joinable()) consumer_thread_.join();
        LogItem* item = nullptr;
        while (queue_.pop(item)) {
            if (item) item_pool_->release(item);
        }
        std::cout.flush();
    }

    void consumeLoop() {
        std::string log_path = std::getenv("LOG_PATH") ? std::getenv("LOG_PATH") : "app.log";
        int fd = -1;
        try {
            std::filesystem::create_directories(std::filesystem::path(log_path).parent_path());
            fd = ::open(log_path.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
        } catch(...) {
            std::cerr << "Failed to create log directory for: " << log_path << "\n";
        }
        if (fd < 0) {
            std::cerr << "Failed to open log file: " << std::getenv("LOG_PATH") << "\n";
            return;
        }
        static const int BATCH_SIZE = 128;  
        std::vector<std::string> buf_batch;
        buf_batch.reserve(BATCH_SIZE);

        std::vector<LogItem*> recycle_batch;
        recycle_batch.reserve(BATCH_SIZE);
        
        while (running_.load(std::memory_order_acquire) || !queue_.empty()) {
            bool processed = false;
            LogItem* item;
            while (queue_.pop(item)) {
                processed = true;
                buf_batch.push_back(formatLog(*item));
                recycle_batch.push_back(item);
                if (buf_batch.size() >= BATCH_SIZE) {
                    flushBatch(fd, buf_batch, recycle_batch);
                }
            }
            if (!buf_batch.empty()) {
                flushBatch(fd, buf_batch, recycle_batch);
                processed = true;
            }
            if (!processed) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
        ::close(fd);
    }

    void flushBatch(int fd, std::vector<std::string>& buf_batch, std::vector<LogItem*>& recycle_batch) {
        std::vector<struct iovec> iovecs(buf_batch.size());
        for (size_t i = 0; i < buf_batch.size(); ++i) {
            iovecs[i].iov_base = static_cast<void*>(buf_batch[i].data());
            iovecs[i].iov_len = buf_batch[i].size();
        }
        ssize_t nwritten = ::writev(fd, iovecs.data(), iovecs.size());
        if (nwritten < 0) {
            std::cerr << "Failed to write log batch to file\n";
        }
        for (auto item : recycle_batch) {
            item_pool_->release(item);
        }
        buf_batch.clear();
        recycle_batch.clear();
    }

    void gettime(char* buf, size_t buf_size) {
        auto now = std::chrono::system_clock::now();
        auto t = std::chrono::system_clock::to_time_t(now);
        std::tm tm_buf{};
        localtime_r(&t, &tm_buf);
        strftime(buf, buf_size, "%Y-%m-%d %H:%M:%S", &tm_buf);
    }

    void printLog(const LogItem& item) {
        std::stringstream ss;
        ss << "[" << item.timestamp << "] "
           << "[" << item.producer_tid << "] "
           << "[" << levelToString(item.level) << "] "
           << item.msg << "\n";
        std::cout << ss.str();
        static thread_local size_t counter = 0;
        if ((++counter & 0xFF) == 0) {
            std::cout.flush();
        }
    }

    std::string formatLog(const LogItem& item) {
        std::stringstream ss;
        ss << "[" << item.timestamp << "] "
           << "[" << item.producer_tid << "] "
           << "[" << levelToString(item.level) << "] "
           << item.msg << "\n";
        return ss.str();
    }

    const char* levelToString(LogLevel level) {
        switch (level) {
            case LogLevel::DEBUG: return "DEBUG";
            case LogLevel::INFO: return "INFO";
            case LogLevel::WARN: return "WARN";
            case LogLevel::ERROR: return "ERROR";
        }
        return "UNKNOWN";
    }
private:
    class LogItemPool {
        public:
            LogItemPool(size_t n) {
                pool_.reserve(n);
                for (size_t i = 0; i < n; ++i) {
                    pool_.push_back(new LogItem());
                }
            }

            ~LogItemPool() {
                std::lock_guard<std::mutex> lock(mutex_);
                for (auto p : pool_) delete p;
            }

            LogItem* acquire() {
                std::lock_guard<std::mutex> lock(mutex_);
                if (pool_.empty()) return new LogItem();
                LogItem* p = pool_.back();
                pool_.pop_back();
                return p;
            }

            void release(LogItem* p) {
                p->msg.clear();
                p->timestamp.clear();
                p->producer_tid = 0;
                std::lock_guard<std::mutex> lock(mutex_);
                pool_.push_back(p);
            }

        private:
            std::vector<LogItem*> pool_;
            std::mutex mutex_;
    };

    boost::lockfree::queue<LogItem*> queue_;
    std::unique_ptr<LogItemPool> item_pool_;
    std::atomic<bool> running_;
    std::thread consumer_thread_;
    static thread_local char time_buf[32];
};
} // namespace LOG
