#pragma once
#include "TCPDataSender.hpp"
#include <atomic>

namespace TCPDataTransfer {
class TCPDataSenderImpl : public TCPDataSender {
public:
    TCPDataSenderImpl();
    ~TCPDataSenderImpl();
    bool open(const std::string& ip, int port) override;
    bool send(const char* data, size_t length) override;
    bool close() override;
private:
    void optimizeSocket();
private:
    int socketfd_;
    std::atomic_bool isOpen_;
};

std::shared_ptr<TCPDataSender> TCPDataSender::create() {
    return std::make_shared<TCPDataSenderImpl>();
}
}