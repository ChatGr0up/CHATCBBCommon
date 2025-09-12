#include <sys/epoll.h>
#include <unistd.h>
#include <sys/socket.h>
#include <shared_mutex>
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
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = socketFd;
    if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, socketFd, &event) == -1) {
        LOG_ERROR("EpollConsumer" << consumerTag_ << ", failed to add socket fd " << socketFd << ", for user " << userId);
        return false;
    }
    {
        std::unique_lock<std::shared_mutex> lock(lastEpollSocketStatusMapMutex_);
        lastEpollSocketStatusMap_[socketFd] = event.events;
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
                {
                    std::lock_guard<std::mutex> lock(pendingDataMutex_);
                    auto it = pendingDataMap_.find(fd);
                    if (it == pendingDataMap_.end() || it->second.empty()) {
                        modifiledEpollToJustListen(fd);
                        continue;
                    }
                    bool allSent = true;
                    for (auto& dataPair : it->second) {
                        if (dataPair.allSent) {
                            continue; 
                        }
                        const std::string& data = dataPair.data;
                        size_t len = dataPair.len - dataPair.index;
                        ssize_t bytesSent = ::send(fd, data.data() + dataPair.index, len, 0);
                        if (bytesSent == -1) {
                            LOG_ERROR("EpollConsumer" << consumerTag_ << ", failed to send data on fd " << fd << ": " << strerror(errno));
                            allSent = false;
                            continue; 
                        }
                        if (bytesSent < static_cast<ssize_t>(len)) {
                            dataPair.index += bytesSent;
                            LOG_INFO("EpollConsumer" << consumerTag_ << ", partial send " << bytesSent << " bytes on fd " << fd);
                            allSent = false;
                            continue; 
                        }
                        dataPair.allSent = true;
                        LOG_INFO("EpollConsumer" << consumerTag_ << ", successfully sent ALL " << bytesSent << " bytes on fd " << fd);
                    }
                    if (allSent) {
                        it->second.clear();
                        modifiledEpollToJustListen(fd);
                        pendingDataMap_.erase(it); 
                    }
                }
            }
            if (eventFlags & (EPOLLHUP | EPOLLERR)) {
                LOG_WARNING("EpollConsumer" << consumerTag_ << ", hang up or error on fd " << fd);
                epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, nullptr);
                ::close(fd);
            }

        }
    }
}  

void EpollConsumer::modifiledEpollToJustListen(int socketFd)
{
    epoll_event event{};
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = socketFd;
    if (epoll_ctl(epollFd_, EPOLL_CTL_MOD, socketFd, &event) == -1) {
        LOG_ERROR("EpollConsumer" << consumerTag_ << ", failed to modify socket fd " << socketFd << " to just listen for EPOLLIN");
    } else {
        {
            std::unique_lock<std::shared_mutex> lock(lastEpollSocketStatusMapMutex_);
            lastEpollSocketStatusMap_[socketFd] = event.events;
        }
        LOG_INFO("EpollConsumer" << consumerTag_ << ", successfully modified socket fd " << socketFd << " to just listen for EPOLLIN");
    }
    {
        std::lock_guard<std::mutex> lock(pendingDataMutex_);
        pendingDataMap_.erase(socketFd);
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

bool EpollConsumer::removeUserSocket(int socketFd, uint64_t userId)
{
    if (epoll_ctl(epollFd_, EPOLL_CTL_DEL, socketFd, nullptr) == -1) {
        LOG_ERROR("EpollConsumer" << consumerTag_ << ", failed to remove socket fd " << socketFd << ", for user " << userId);
        ::close(socketFd);
        return false;
    } 
    LOG_INFO("EpollConsumer" << consumerTag_ << ", removed socket fd " << socketFd << ", for user " << userId);
    ::close(socketFd);
    {
        std::lock_guard<std::mutex> lock(pendingDataMutex_);
        pendingDataMap_.erase(socketFd);
    }
    {
        std::unique_lock<std::shared_mutex> lock(lastEpollSocketStatusMapMutex_);
        lastEpollSocketStatusMap_.erase(socketFd);
    }
    return true;
}

bool EpollConsumer::sendData(int socketFd, uint64_t connId, const char* data, size_t len)
{
    LOG_INFO("EpollConsumer" << consumerTag_ << ", sendData called for socketFd " << socketFd << ", connId " << connId << ", data length " << len);
    {
        std::lock_guard<std::mutex> lock(pendingDataMutex_);
        pendingDataMap_[socketFd].emplace_back(std::string(data), len);
    }
    decltype(lastEpollSocketStatusMap_)::iterator it;
    {
        std::shared_lock<std::shared_mutex> lock(lastEpollSocketStatusMapMutex_);
        it = lastEpollSocketStatusMap_.find(socketFd);
        if (it != lastEpollSocketStatusMap_.end() && (it->second & EPOLLOUT)) {
            LOG_INFO("EpollConsumer" << consumerTag_ << ", EPOLLOUT already set for socketFd " << socketFd);
            return true;
        }
    }
    epoll_event event{};
    event.events = EPOLLIN | EPOLLOUT | EPOLLET;
    event.data.fd = socketFd;
    if (epoll_ctl(epollFd_, EPOLL_CTL_MOD, socketFd, &event) == -1) {
        LOG_ERROR("EpollConsumer" << consumerTag_ << ", failed to modify socket fd " << socketFd << " to add EPOLLOUT");
        return false;
    }
    {
        std::unique_lock<std::shared_mutex> lock(lastEpollSocketStatusMapMutex_);
        lastEpollSocketStatusMap_[socketFd] = event.events;
    }
    LOG_INFO("EpollConsumer" << consumerTag_ << ", successfully EPOLLOUT added for socketFd " << socketFd);
    return true;
}
}