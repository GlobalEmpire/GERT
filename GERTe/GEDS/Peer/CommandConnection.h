#pragma once
#include "../Networking/Connection.h"
#include "CommandPacket.h"
#include <vector>

class CommandConnection: public Connection {
    std::vector<Address> clients;
    std::vector<DataPacket> queued;
    CommandPacket* curPacket = nullptr;

public:
    explicit CommandConnection(SOCKET);
    explicit CommandConnection(SOCKET, void*);
    ~CommandConnection() noexcept override;

    void process() override;
    void send(const DataPacket&) override;

    static void attempt(const DataPacket&);
};
