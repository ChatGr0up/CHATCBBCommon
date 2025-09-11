#pragma once

#include "ConnectionDef.hpp"
#include "EpollConsumerPool.hpp"
#include <unordered_map>
#include <atomic>
#include <shared_mutex>

namespace TCPDataTransfer {
class TCPDataTransfer {
public:
    static TCPDataTransfer& instance();

    connectInfo buildConnection(uint64_t userId);
    void removeConnection(uint64_t connId);
    bool sendData(uint64_t connId, const char* data, size_t len);
    bool isConnectionAlive(uint64_t connId);
private:
    TCPDataTransfer();
    ~TCPDataTransfer();
    void init();
    bool buildSocket(connectInfo& conn);
    bool optimizeSocket(int socketfd_);
private:
    std::shared_mutex connMutex_;
    std::unordered_map<uint64_t, connectInfo> connections_;
    std::atomic<uint64_t> connNum{0};
    std::unique_ptr<EpollConsumerPool> epollConsumerPool_;
};
}