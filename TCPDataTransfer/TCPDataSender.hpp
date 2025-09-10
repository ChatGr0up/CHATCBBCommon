#pragma once

#include <cstddef>
#include <memory>
#include "TCPDataSenderImpl.hpp"

namespace TCPDataTransfer {
class TCPDataSender {
public:
    virtual bool open(const std::string& ip, int port) = 0;
    virtual bool send(const char* data, size_t length) = 0;
    virtual bool close() = 0;
    virtual ~TCPDataSender() = default;
    
    static std::shared_ptr<TCPDataSender> create();
};
}
