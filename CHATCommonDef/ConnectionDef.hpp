#pragma once

#include <cstdint>
#include <string>
#include <cstddef>

namespace TCPDataTransfer {
struct connectInfo {
    uint64_t connId; // equal to userId
    int socketFd;
    std::string clientIp;
    uint16_t clientPort;
    uint16_t serverPort;
    uint32_t lastActiveTime;
};

const uint64_t MAX_CONNECTIONS = 25000;
const uint64_t SOMAXCONN = 4096; // by setting /proc/sys/net/core/somaxconn
}