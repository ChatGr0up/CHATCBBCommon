#include "TCPDataSenderImpl.hpp"
#include "LogMacro.hpp"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/tcp.h>

namespace TCPDataTransfer {
TCPDataSenderImpl::TCPDataSenderImpl() : socketfd_(-1), isOpen_(false)
{
    LOG_INFO("TCPDataSenderImpl created.");
}

TCPDataSenderImpl::~TCPDataSenderImpl()
{
    LOG_INFO("TCPDataSenderImpl destroyed.");
}

bool TCPDataSenderImpl::open(const std::string& ip, int port)
{
    if (isOpen_) {
        LOG_ERROR("Socket is already open.");
        return true;
    }
    socketfd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socketfd_ == -1) {
        LOG_ERROR("Failed to create socket.");
        return false;
    }
    optimizeSocket();
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &serverAddr.sin_addr);
    if (connect(socketfd_, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        LOG_ERROR("Connection to server failed.");
        close();
        return false;
    }
    isOpen_ = true;
    LOG_INFO("Connected to server, ip: " << ip << ", port: " << port);
    return true;
}

void TCPDataSenderImpl::optimizeSocket()
{
    int opt = 1; // no Nagle
    setsockopt(socketfd_, IPPROTO_TCP, TCP_NODELAY, (char*)&opt, sizeof(opt));

    int sndBuf = 1 << 20; // 1MB
    setsockopt(socketfd_, SOL_SOCKET, SO_SNDBUF, &sndBuf, sizeof(sndBuf));

    fcntl(socketfd_, F_SETFL, O_NONBLOCK); // non-blocking

    struct timeval timeout{3, 0}; 
    setsockopt(socketfd_, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)); // Send timeout

    int reuse = 1;
    setsockopt(socketfd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    setsockopt(socketfd_, IPPROTO_IP, IP_BIND_ADDRESS_NO_PORT, &opt, sizeof(opt)); // allow binding to an address without a port
}

bool TCPDataSenderImpl::send(const char* data, size_t length)
{
    if (!isOpen_) {
        LOG_ERROR("Socket is not open.");
        return false;
    }
    ssize_t bytesSent = ::send(socketfd_, data, length, MSG_NOSIGNAL);
    if (bytesSent < 0) {
        LOG_ERROR("Failed to send data.");
        return false;
    }
    return true;
}

bool TCPDataSenderImpl::close()
{
    if (!isOpen_) {
        LOG_ERROR("Socket is already closed.");
        return true;
    }
    if (socketfd_ != -1) {
        ::close(socketfd_);
        socketfd_ = -1;
    }
    isOpen_ = false;
    LOG_INFO("Socket closed.");
    return true;
}
}