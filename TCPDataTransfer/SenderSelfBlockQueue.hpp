#pragma once

#include <string>
#include <mutex>
#include <queue>
#include <condition_variable>

namespace TCPDataTransfer {
class SenderSelfBlockQueue {
public:
    void pushData(std::string && data) {
        std::lock_guard<std::mutex> lock(mutex_);
        dataQueue_.push(std::move(data));
        dataCond_.notify_one();
    }
    
    bool popData(std::string& data) {
        std::unique_lock<std::mutex> lock(mutex_);
        dataCond_.wait(lock, [this](){return !dataQueue_.empty();});
        data = std::move(dataQueue_.front());
        dataQueue_.pop();
        return true;
    }
private:
    std::mutex mutex_;
    std::queue<std::string> dataQueue_;
    std::conditional_variable dataCond_;
}
}