#pragma once
#include "../Gateway/Address.h"
#include "CommandPacket.h"

class QueryPacket: public CommandPacket {
public:
    Address query;

    QueryPacket();

    bool parse(const std::string&) override;
};
