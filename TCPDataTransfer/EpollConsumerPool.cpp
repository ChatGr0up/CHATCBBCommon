#include "EpollConsumerPool.hpp"
#include "ConnectionDef.hpp"
#include "LogMacro.hpp"

namespace TCPDataTransfer {
EpollConsumerPool::EpollConsumerPool()
{
    LOG_INFO("EpollConsumerPool init with MAX_EPOLL_CONSUMERS: " << MAX_EPOLL_CONSUMERS);
    for (uint16_t i = 0; i < MAX_EPOLL_CONSUMERS; ++i) {
        try {
            epollConsumerMap_.insert({i, std::move(std::make_unique<EpollConsumer>(i))});
        } CATCH_AND_MSG("CONSUMERS INIT FAILED for index: " << i);
    }
}

EpollConsumerPool::~EpollConsumerPool()
{
    for (auto& [id, consumer] : epollConsumerMap_) {
        consumer->stop();
    }
    {
        std::unique_lock<std::shared_mutex> lock(socketUserEpollConsumerMap_mutex);
        for (auto& [key, index] : socketUserEpollConsumerMap_) {
            ::close(key.first);
        }
        socketUserEpollConsumerMap_.clear();
    }
}

bool EpollConsumerPool::addUserSocket(int socketFd, uint64_t userId)
{
    int index = ++userIndex_ % MAX_EPOLL_CONSUMERS;
    if (userIndex_ >= 1000000) {
        userIndex_ = 0;
    }
    auto consumer = epollConsumerMap_.find(index);
    if (consumer == epollConsumerMap_.end()) {
        LOG_ERROR("cannot find epoll consumer for index: " << index);
        return false;
    }
    if (consumer->second->addUserSocket(socketFd, userId)) {
        LOG_INFO("successfully add user socketfd: " << socketFd << " for userId: " << userId << " to epoll consumer index: " << index);
        std::unique_lock<std::shared_mutex> lock(socketUserEpollConsumerMap_mutex);
        socketUserEpollConsumerMap_.insert({std::make_pair(socketFd, userId), index});
        return true;
    } 
    LOG_ERROR("failed to add user socketfd: " << socketFd << " for userId: " << userId << " to epoll consumer index: " << index);
    return false;
}

void EpollConsumerPool::removeUserSocket(int socketFd, uint64_t userId)
{
    decltype(socketUserEpollConsumerMap_)::iterator it;
    decltype(epollConsumerMap_)::iterator consumer;
    int index = -1;
    {
        std::shared_lock<std::shared_mutex> lock(socketUserEpollConsumerMap_mutex);
        it = socketUserEpollConsumerMap_.find({socketFd, userId});
        if (it == socketUserEpollConsumerMap_.end()) {
            LOG_ERROR("cannot find socketFd: " << socketFd << " for userId: " << userId << " in socketUserEpollConsumerMap_");
            return;
        }
        index = it->second;
    }
    consumer = epollConsumerMap_.find(index);
    if (consumer == epollConsumerMap_.end()) {
        LOG_ERROR("cannot find epoll consumer for index: " << index);
        return;
    }
    if (consumer->second->removeUserSocket(socketFd, userId)) {
        LOG_INFO("successfully remove user socketfd: " << socketFd << " for userId: " << userId << " from epoll consumer index: " << index);
        std::unique_lock<std::shared_mutex> lock(socketUserEpollConsumerMap_mutex);
        socketUserEpollConsumerMap_.erase(it);
        return;
    } 
    LOG_ERROR("failed to remove user socketfd: " << socketFd << " for userId: " << userId << " from epoll consumer index: " << index);
}

bool EpollConsumerPool::sendData(int socketFd, uint64_t connId, const char* data, size_t len)
{
    decltype(socketUserEpollConsumerMap_)::iterator it;
    decltype(epollConsumerMap_)::iterator consumer;
    int index = -1;
    {
        std::shared_lock<std::shared_mutex> lock(socketUserEpollConsumerMap_mutex);
        it = socketUserEpollConsumerMap_.find({socketFd, connId});
        if (it == socketUserEpollConsumerMap_.end()) {
            LOG_ERROR("cannot find socketFd: " << socketFd << " for connId: " << connId << " in socketUserEpollConsumerMap_");
            return false;
        }
        index = it->second;
    }
    consumer = epollConsumerMap_.find(index);
    if (consumer == epollConsumerMap_.end()) {
        LOG_ERROR("cannot find epoll consumer for index: " << index);
        return false;
    }
    if (consumer->second->sendData(socketFd, connId, data, len)) {
        return true;
    } 
    LOG_ERROR("failed to send data to socketFd: " << socketFd << " for connId: " << connId << " from epoll consumer index: " << index);
    return false;
}
}