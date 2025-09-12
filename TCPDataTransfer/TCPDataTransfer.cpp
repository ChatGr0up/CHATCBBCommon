#include "TCPDataTransfer.hpp"
#include "ConnectionDef.hpp"
#include "LogMacro.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/tcp.h>
#include <thread>

namespace TCPDataTransfer {
TCPDataTransfer& TCPDataTransfer::instance() {
    static TCPDataTransfer instance;
    return instance;
}

TCPDataTransfer::TCPDataTransfer()
{
    LOG_INFO("TCPDataTransfer constructor called.");
    init();
}

TCPDataTransfer::~TCPDataTransfer()
{
    LOG_INFO("TCPDataTransfer destructor called.");
}

connectInfo TCPDataTransfer::buildConnection(uint64_t userId, const std::string& clientIp, int clientPort)
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
    if (!buildSocket(newConn, clientIp, clientPort)) {
        connNum--;  
        return connectInfo{};
    }
    if (!epollConsumerPool_->addUserSocket(newConn.socketFd, userId)) {
        ::close(newConn.socketFd);
        connNum--;  
        return connectInfo{};
    }
    newConn.connId = userId;
    {
        std::unique_lock<std::shared_mutex> locl(connMutex_);
        connections_[userId] = newConn;
    }
    return newConn; 
}

void TCPDataTransfer::removeConnection(uint64_t connId)
{
    std::unique_lock<std::shared_mutex> locl(connMutex_);
    auto it = connections_.find(connId);
    if (it == connections_.end()) {
        LOG_ERROR("Connection for connId " << connId << " not found. REMOVE CONNECTION FAILED.");
        return;
    }
    epollConsumerPool_->removeUserSocket(it->second.socketFd, connId);
    connections_.erase(it);
    connNum--;
    LOG_INFO("Connection for userId " << connId << " removed.");
} 

bool TCPDataTransfer::buildSocket(connectInfo& conn, const std::string& clientIp, int clientPort)
{
    auto userSocket = ::socket(AF_INET, SOCK_STREAM, 0);
    if (userSocket < 0) {
        LOG_ERROR("Socket creation failed.");
        return false;
    }
    setSocketNonBlocking(userSocket);
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
    struct sockaddr_in clientAddr{};
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_port = htons(clientPort);
    if (inet_pton(AF_INET, clientIp.c_str(), &clientAddr.sin_addr) <= 0) {
        LOG_ERROR("Invalid client IP address: " << clientIp << ", faield to bind sender socket.");
        ::close(userSocket);
        return false;   
    }
    if (::connect(userSocket, (struct sockaddr*)&clientAddr, sizeof(clientAddr)) < 0) {
        if (errno != EINPROGRESS) { 
            LOG_ERROR("Socket connect to client for socket " << userSocket << ", clientIp " << clientIp << " failed.");
            ::close(userSocket);
            return false;
        } else {
            LOG_INFO("Socket connect in progress for socket " << userSocket << ", clientIp " << clientIp << ", wating for consumer thread to handle it.");
        }
    }
    conn.socketFd = userSocket;
    conn.serverPort = assignedPort;
    return true;
}

bool TCPDataTransfer::setSocketNonBlocking(int socketfd)
{
    int flags = fcntl(socketfd, F_GETFL, 0);
    if (flags == -1) {
        LOG_ERROR("fcntl F_GETFL failed.");
        return false;
    }
    
    if (fcntl(socketfd, F_SETFL, flags | O_NONBLOCK) == -1) {
        LOG_ERROR("fcntl O_NONBLOCK failed.");
        return false;
    }

    return true;
}

bool TCPDataTransfer::optimizeSocket(int socketfd_)
{
    int opt = 1; // no Nagle
    if (setsockopt(socketfd_, IPPROTO_TCP, TCP_NODELAY, (char*)&opt, sizeof(opt)) < 0) {
        LOG_ERROR("setsockopt TCP_NODELAY failed.");
        return false;
    }

    int sndBuf = 1 << 20; // 1MB
    if (setsockopt(socketfd_, SOL_SOCKET, SO_SNDBUF, &sndBuf, sizeof(sndBuf)) < 0) {
        LOG_ERROR("setsockopt SO_SNDBUF failed.");
        return false;
    }

    struct timeval timeout{3, 0}; 
    if (setsockopt(socketfd_, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0) {
        LOG_ERROR("setsockopt SO_SNDTIMEO failed.");
        return false;
    } // Send timeout

    int reuse = 1;
    if (setsockopt(socketfd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        LOG_ERROR("setsockopt SO_REUSEADDR failed.");
        return false;
    }

    if (setsockopt(socketfd_, IPPROTO_IP, IP_BIND_ADDRESS_NO_PORT, &opt, sizeof(opt)) < 0){
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

bool TCPDataTransfer::sendData(int socketFd, uint64_t connId, const char* data, size_t len)
{
    {
        std::shared_lock<std::shared_mutex> locl(connMutex_);
        auto it = connections_.find(connId);
        if (it == connections_.end()) {
            LOG_ERROR("Connection for connId " << connId << " not found. SEND DATA FAILED.");
            return false;
        }
    }
    if (epollConsumerPool_->sendData(socketFd, connId, data, len)) {
        LOG_INFO("EpollConsumerPool sent data for connId " << connId << ", socket " << socketFd << ", data length: " << len);
        return true;
    } 
    LOG_ERROR("EpollConsumerPool failed to send data for connId " << connId);
    return false;
}
}