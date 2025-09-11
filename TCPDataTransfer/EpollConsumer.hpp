#pragma once

namespace TCPDataTransfer {
class EpollConsumer {
public:
    explicit EpollConsumer(int consumerTag);
    ~EpollConsumer();

    void stop();

};
}