#pragma once

#include "ConnectionDef.hpp"
#include "EpollConsumerPool.hpp"
#include <unordered_map>
#include <atomic>
#include <shared_mutex>
#include <queue>
#include <netinet/in.h>

namespace TCPDataTransfer {
class TCPDataTransfer {
public:
    static TCPDataTransfer& instance();

    connectInfo buildConnection(uint64_t userId, const std::string& clientIp, int clientPort);
    void removeConnection(uint64_t connId);
    bool sendData(int socketFd, uint64_t connId, const char* data, size_t len);
private:
    TCPDataTransfer();
    ~TCPDataTransfer();
    void init();
    bool buildSocket(connectInfo& conn, const std::string& clientIp, int clientPort);
    bool optimizeSocket(int socketfd_);
    void loopForConnection();
    bool setSocketNonBlocking(int socketfd);
private:
    std::shared_mutex connMutex_;
    std::unordered_map<uint64_t, connectInfo> connections_;
    std::atomic<uint64_t> connNum{0};
    std::unique_ptr<EpollConsumerPool> epollConsumerPool_;
    std::map<uint64_t, bool> isUserIdConnected_;
};
}