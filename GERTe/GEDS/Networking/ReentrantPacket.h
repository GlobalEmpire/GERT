#pragma once
#include <string>

class ReentrantPacket {
protected:
    int need = 0;
    std::string partial{};

    ReentrantPacket() = default;

public:
    std::string raw{};

    [[nodiscard]] int needed() const;
    virtual bool parse(const std::string&) = 0;
};
