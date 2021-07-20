#pragma once
#include "../Networking/ReentrantPacket.h"
#include "GERTc.h"

class DataPacket: public ReentrantPacket {
    unsigned char sigLength = 0;
    unsigned short length = 0;

    bool stamped = false;
    bool lengthed = false;

public:
    DataPacket();

    GERTc source{};
    GERTc destination{};
    std::string data{};
    unsigned long timestamp = 0;
    std::string signature{};

    bool parse(const std::string&) override;
};
