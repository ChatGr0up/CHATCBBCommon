#pragma once

#include <atomic>
#include <thread>
#include <map>
#include <vector>
#include <string>
#include <mutex>
#include <chrono>
#include <shared_mutex>

namespace TCPDataTransfer {
struct pendingData {
    std::string data;
    size_t len;
    size_t index{0};
    bool allSent{false};
    pendingData(const std::string& d, size_t l) : data(d), len(l) {}
};
class EpollConsumer {
public:
    explicit EpollConsumer(int consumerTag);
    ~EpollConsumer();

    void stop();
    bool addUserSocket(int socketFd, uint64_t userId);
    bool removeUserSocket(int socketFd, uint64_t userId);
    bool sendData(int socketFd, uint64_t connId, const char* data, size_t len);
private:
    void start();
    void run();
    void restartEpollConsumer();
    void modifiledEpollToJustListen(int socketFd);
private:
    int consumerTag_;
    int epollFd_;
    std::atomic_bool isRunning_;
    std::atomic_int32_t errorCount_{0};
    std::thread consumerThread_;
    std::map<int, std::vector<pendingData>> pendingDataMap_; 
    std::mutex pendingDataMutex_;
    std::map<int, uint32_t> lastEpollSocketStatusMap_;
    std::shared_mutex lastEpollSocketStatusMapMutex_;
};
}