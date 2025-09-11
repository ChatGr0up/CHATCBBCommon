#include "TCPDataTransfer.hpp"
#include "ConnectionDef.hpp"
#include "LogMacro.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/tcp.h>

namespace TCPDataTransfer {
TCPDataTransfer& TCPDataTransfer::instance() {
    static TCPDataTransfer instance;
    return instance;
}

TCPDataTransfer::TCPDataTransfer()
{
    init();
}

TCPDataTransfer::~TCPDataTransfer()
{

}

connectInfo TCPDataTransfer::buildConnection(uint64_t userId)
{
    if (connNum >= MAX_CONNECTIONS) {
        LOG_ERROR("Too many connections, cannot build new connection in this server.");
        return connectInfo{}; 
    }
    {
        std::shared_lock<std::shared_mutex> locl(connMutex_);
        if (connections_.find(userId) != connections_.end()) {
            LOG_WARNING("Connection for userId " << userId << ", already exists.");
            return connections_[userId];
        }
    }
    connNum++;
    connectInfo newConn;
    if (!buildSocket(newConn)) {
        connNum--;  
        return connectInfo{};
    }
    if (!epollConsumerPool_->addUserSocket(newConn.socketFd, userId)) {
        ::close(newConn.socketFd);
        connNum--;  
        return connectInfo{};
    }
    return newConn; 
}

bool TCPDataTransfer::buildSocket(connectInfo& conn)
{
    auto userSocket = ::socket(AF_INET, SOCK_STREAM, 0);
    if (userSocket < 0) {
        LOG_ERROR("Socket creation failed.");
        return false;
    }

    struct sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = 0;
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(userSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        LOG_ERROR("Socket bind failed.");
        ::close(userSocket);
        return false;
    }

    struct sockaddr_in assignedAddr{};
    socklen_t addrLen = sizeof(assignedAddr);
    if (getsockname(userSocket, (struct sockaddr*)&assignedAddr, &addrLen) < 0) {
        LOG_ERROR("getsockname failed.");
        ::close(userSocket);
        return false;   
    }
    auto assignedPort = ntohs(assignedAddr.sin_port);
    LOG_INFO("Socket assigned port: " << assignedPort);
    if (listen(userSocket, SOMAXCONN) < 0) {
        LOG_ERROR("Socket listen failed.");
        ::close(userSocket);
        return false;   
    }
    if (!optimizeSocket(userSocket)) {
        ::close(userSocket);
        return false;   
    }
    conn.socketFd = userSocket;
    conn.serverPort = assignedPort;
    return true;
}

bool TCPDataTransfer::optimizeSocket(int socketfd_)
{
    int opt = 1; // no Nagle
    if (!setsockopt(socketfd_, IPPROTO_TCP, TCP_NODELAY, (char*)&opt, sizeof(opt))) {
        LOG_ERROR("setsockopt TCP_NODELAY failed.");
        return false;
    }

    int sndBuf = 1 << 20; // 1MB
    if (!setsockopt(socketfd_, SOL_SOCKET, SO_SNDBUF, &sndBuf, sizeof(sndBuf))) {
        LOG_ERROR("setsockopt SO_SNDBUF failed.");
        return false;
    }

    if (!fcntl(socketfd_, F_SETFL, O_NONBLOCK)) {
        LOG_ERROR("fcntl O_NONBLOCK failed.");
        return false;
    } // non-blocking

    struct timeval timeout{3, 0}; 
    if (!setsockopt(socketfd_, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout))) {
        LOG_ERROR("setsockopt SO_SNDTIMEO failed.");
        return false;
    } // Send timeout

    int reuse = 1;
    if (!setsockopt(socketfd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse))) {
        LOG_ERROR("setsockopt SO_REUSEADDR failed.");
        return false;
    }

    if (!setsockopt(socketfd_, IPPROTO_IP, IP_BIND_ADDRESS_NO_PORT, &opt, sizeof(opt))){
        LOG_ERROR("setsockopt IP_BIND_ADDRESS_NO_PORT failed.");
        return false;
    } // allow binding to an address without a port
    LOG_INFO("Socket optimized successfully, socketfd: " << socketfd_);
    return true;
}

void TCPDataTransfer::init()
{
    LOG_INFO("TCPDataTransfer initialized.");
    epollConsumerPool_ = std::make_unique<EpollConsumerPool>();
}
}