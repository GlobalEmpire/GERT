#pragma once
#include "Connection.h"
#include "../Gateway/GERTc.h"
#include "DataPacket.h"
#include "Logic.h"

class SecureConnection : public Connection {
    std::string secret;
    DataPacket curPacket{};

    Logic* processor;

public:
    Address addr;

    SecureConnection(SOCKET, const std::string&, Logic*);

    void send(const DataPacket&);

    void process() override;
};
