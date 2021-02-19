#pragma once

#include "CommandPacket.h"
#include "../Gateway/Address.h"

class QueryPPacket: public CommandPacket {
public:
    Address query;
    unsigned char major = 0;
    unsigned char minor = 0;
    bool direct = false;

    QueryPPacket();

    bool parse(const std::string&) override;
};
