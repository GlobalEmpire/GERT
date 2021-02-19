#pragma once
#include "../Networking/ReentrantPacket.h"
#include "GERTc.h"

class DataPacket: public ReentrantPacket {
public:
    DataPacket();

    GERTc source{};
    GERTc destination{};
    std::string data{};
    unsigned long timestamp = 0;
    std::string signature{};

    bool parse(const std::string&) override;
};
