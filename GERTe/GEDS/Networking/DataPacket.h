#pragma once
#include "../Gateway/GERTc.h"

class DataPacket {
    std::string partial{};

public:
    GERTc source{};
    GERTc destination{};
    std::string data{};
    std::string hmac{};
    std::string raw{};

    [[nodiscard]] int needed() const;
    bool parse(const std::string&);
};
