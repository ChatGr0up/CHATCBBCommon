#include "EpollConsumerPool.hpp"
#include "ConnectionDef.hpp"
#include "LogMacro.hpp"

namespace TCPDataTransfer {
EpollConsumerPool::EpollConsumerPool()
{
    LOG_INFO("EpollConsumerPool init with MAX_EPOLL_CONSUMERS: " << MAX_EPOLL_CONSUMERS);
    try {
        for (uint16_t i = 0; i < MAX_EPOLL_CONSUMERS; ++i) {
            epollConsumerMap_.insert({i, std::move(std::make_unique<EpollConsumer>(i))});
        }
    } CATCH_DEFAULT
    
}

EpollConsumerPool::~EpollConsumerPool()
{
    for (auto& [id, consumer] : epollConsumerMap_) {
        consumer->stop();
    }
}


bool EpollConsumerPool::addUserSocket(int socketFd, uint64_t userId)
{
    int index = userId % MAX_EPOLL_CONSUMERS;
    auto consumer = epollConsumerMap_.find(index);
    if (consumer == epollConsumerMap_.end()) {
        LOG_ERROR("cannot find epoll consumer for index: " << index);
        return false;
    }
    return consumer->second->addUserSocket(socketFd, userId);
}
}