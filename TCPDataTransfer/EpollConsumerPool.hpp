#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include "EpollConsumer.hpp"

namespace TCPDataTransfer {
class EpollConsumerPool {
public:
    EpollConsumerPool();
    ~EpollConsumerPool();

    bool start();
    bool stop();
    bool addUserSocket(int socketFd, uint64_t userId);
private:
    std::map<uint16_t, std::unique_ptr<EpollConsumer>> epollConsumerMap_;
    std::map<uint16_t, uint64_t> epollConsumerConnectNumMap_;
};
}