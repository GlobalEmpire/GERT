#pragma once
#include "../Networking/Socket.h"
#include <string>
#include <vector>

class Config {
public:
    std::vector<std::pair<uint32_t, uint16_t>> peers4;
    std::vector<std::pair<ipv6, uint16_t>> peers6;

    explicit Config(const std::string&);

    static std::string stringify(uint32_t);
    static std::string stringify(ipv6);
};
