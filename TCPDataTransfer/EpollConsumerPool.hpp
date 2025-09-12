#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include "EpollConsumer.hpp"
#include <atomic>
#include <shared_mutex>

namespace TCPDataTransfer {
class EpollConsumerPool {
public:
    EpollConsumerPool();
    ~EpollConsumerPool();
    bool addUserSocket(int socketFd, uint64_t userId);
    void removeUserSocket(int socketFd, uint64_t userId);
    bool sendData(int socketFd, uint64_t connId, const char* data, size_t len);
private:
    bool start();
    bool stop();
private:
    std::map<uint16_t, std::unique_ptr<EpollConsumer>> epollConsumerMap_;
    std::map<uint16_t, uint64_t> epollConsumerConnectNumMap_;
    std::atomic<uint32_t> userIndex_;
    std::map<std::pair<int, uint64_t>, uint16_t> socketUserEpollConsumerMap_;
    std::shared_mutex socketUserEpollConsumerMap_mutex;
};
}