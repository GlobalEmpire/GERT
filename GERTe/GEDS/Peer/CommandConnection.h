#pragma once
#include "../Networking/Connection.h"
#include "CommandPacket.h"
#include <vector>

class CommandConnection: public Connection {
    std::vector<Address> clients;
    CommandPacket* curPacket = nullptr;

public:
    explicit CommandConnection(SOCKET s);
    ~CommandConnection() noexcept override;

    void process() override;
    void send(const DataPacket&) override;
};
