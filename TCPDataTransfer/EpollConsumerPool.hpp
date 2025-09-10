#pragma once

#include <cstdint>
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

};
}