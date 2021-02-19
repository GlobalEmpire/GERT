#pragma once
#include "../Networking/Connection.h"
#include "GERTc.h"
#include "DataPacket.h"

class DataConnection : public Connection {
    DataPacket curPacket{};
    void* key;

public:
    Address addr;

    explicit DataConnection(SOCKET);
    ~DataConnection() noexcept override;

    void process() override;
    void send(const DataPacket&) override;
};
