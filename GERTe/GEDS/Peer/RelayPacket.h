#pragma once

#include "CommandPacket.h"
#include "../Gateway/GERTc.h"

class RelayPacket: public CommandPacket {
public:
    GERTc source{};
    GERTc destination{};
    std::string data{};
    unsigned long timestamp = 0;
    std::string signature{};
    std::string subraw{};

    bool parse(const std::string&) override;

    RelayPacket();
};
