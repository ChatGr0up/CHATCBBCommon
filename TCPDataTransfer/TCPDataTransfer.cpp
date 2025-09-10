#include "TCPDataTransfer.hpp"
#include "ConnectionDef.hpp"
#include "LogMacro.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

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
    if (!epollConsumerPool_.addUserSocket(newConn.socketFd, userId)) {
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
    conn.socketFd = userSocket;
    conn.serverPort = assignedPort;
    return true;
}

void TCPDataTransfer::init()
{
    LOG_INFO("TCPDataTransfer initialized.");
}
}