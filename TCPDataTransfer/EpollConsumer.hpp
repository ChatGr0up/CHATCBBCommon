#pragma once

#include <atomic>
#include <thread>

namespace TCPDataTransfer {
class EpollConsumer {
public:
    explicit EpollConsumer(int consumerTag);
    ~EpollConsumer();

    void stop();
    bool addUserSocket(int socketFd, uint64_t userId);
private:
    void start();
    void run();
    void restartEpollConsumer();
private:
    int consumerTag_;
    int epollFd_;
    std::atomic_bool isRunning_;
    std::atomic_int32_t errorCount_{0};
    std::thread consumerThread_;
};
}