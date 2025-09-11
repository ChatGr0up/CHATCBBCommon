#include "EpollConsumerPool.hpp"
#include "ConnectionDef.hpp"
namespace TCPDataTransfer {
EpollConsumerPool::EpollConsumerPool()
{
    for (uint16_t i = 0; i < MAX_EPOLL_CONSUMERS; ++i) {
        epollConsumerMap_.insert({i, std::move(std::make_unique<EpollConsumer>(i))});
    }
}

EpollConsumerPool::~EpollConsumerPool()
{
    for (auto& [id, consumer] : epollConsumerMap_) {
        consumer->stop();
    }
}


bool EpollConsumerPool::addUserSocket(int socketFd, uint64_t userId)
{
    return true;
}
}