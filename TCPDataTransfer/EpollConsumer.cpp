#include <sys/epoll.h>
#include "EpollConsumer.hpp"
#include "LogMacro.hpp"

namespace TCPDataTransfer {
EpollConsumer::EpollConsumer(int consumerTag) : consumerTag_(consumerTag), epollFd_(-1), isRunning_(false) 
{
    LOG_INFO("EpollConsumer" << consumerTag_ << ", created");
    start();
}

void EpollConsumer::start()
{
    epollFd_ = epoll_create1(0);
    if (epollFd_ == -1) {
        LOG_ERROR("EpollConsumer" << consumerTag_ << " failed to create epoll instance");
        return;
    }
    isRunning_ = true;
    consumerThread_ = std::thread(&EpollConsumer::run, this);
    LOG_INFO("EpollConsumer" << consumerTag_ << " started, thread id: " << consumerThread_.get_id());
}

bool EpollConsumer::addUserSocket(int socketFd, uint64_t userId)
{
    epoll_event event{};
    event.events = EPOLLIN;
    event.data.fd = socketFd;
    if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, socketFd, &event) == -1) {
        LOG_ERROR("EpollConsumer" << consumerTag_ << ", failed to add socket fd " << socketFd << ", for user " << userId);
        return false;
    }
    return true;
}

void EpollConsumer::run()
{
    epoll_event events[1024];
    while (isRunning_) {
        int eventCount = epoll_wait(epollFd_, events, 1024, -1);
        if (eventCount == 0) {
            LOG_INFO("EpollConsumer" << consumerTag_ << ", epoll_wait timeout, normal situation");
            continue; 
        }
        if (eventCount == -1) {
            if (errno == EINTR) {
                continue; 
            }
            LOG_ERROR("EpollConsumer" << consumerTag_ << ", epoll_wait error: " << strerror(errno));
            sleep(1); 
            if (++errorCount_ >= 3) {
                LOG_ERROR("EpollConsumer" << consumerTag_ << ", too many epoll_wait errors, restarting...");
                errorCount_ = 0;
                restartEpollConsumer();
                return;
            }
            continue;
        }
        for (int i = 0; i < eventCount; ++i) {
            int fd = events[i].data.fd;
            uint32_t eventFlags = events[i].events;
            if (eventFlags & EPOLLIN) {
                LOG_INFO("EpollConsumer" << consumerTag_ << ", data available to read on fd " << fd);
                // TODO: 收数据
            }
            if (eventFlags & EPOLLOUT) {
                LOG_INFO("EpollConsumer" << consumerTag_ << ", ready to write on fd " << fd);
                // TODO: 从发送队列取数据写出去
            }
            if (eventFlags & (EPOLLHUP | EPOLLERR)) {
                LOG_WARNING("EpollConsumer" << consumerTag_ << ", hang up or error on fd " << fd);
                epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, nullptr);
                ::close(fd);
            }

        }
    }
}  

void EpollConsumer::restartEpollConsumer()
{
    LOG_INFO("EpollConsumer" << consumerTag_ << " is restarting...");
    stop();
    start();
}

void EpollConsumer::stop()
{
    isRunning_ = false;
    if (epollFd_ != -1) {
        close(epollFd_);
        epollFd_ = -1;
    }
    if (consumerThread_.joinable()) {
        consumerThread_.join();
    }
    LOG_INFO("EpollConsumer" << consumerTag_ << " stopped");
}
}