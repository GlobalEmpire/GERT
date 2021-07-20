#pragma once

#include "CommandPacket.h"
#include "../Gateway/GERTc.h"
#include "../Gateway/DataPacket.h"

class RelayPacket: public CommandPacket {
public:
    DataPacket subpacket;

    bool parse(const std::string&) override;

    RelayPacket();
};
