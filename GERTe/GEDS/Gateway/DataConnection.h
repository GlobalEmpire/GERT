#pragma once
#include "../Networking/Connection.h"
#include "GERTc.h"
#include "DataPacket.h"

class DataConnection : public Connection {
    template<class T>
    friend class Server;

    DataPacket curPacket{};
    void* key = nullptr;

    void _process() override;
    bool handshake() override;

    explicit DataConnection(SOCKET);

public:
    Address addr;

    ~DataConnection() noexcept override;

    void send(const DataPacket&) override;
};
