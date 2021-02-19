#pragma once
#include <string>

class ReentrantPacket {
    int offset;

protected:
    std::string partial{};

    explicit ReentrantPacket(int);

public:
    std::string raw{};

    [[nodiscard]] int needed() const;
    virtual bool parse(const std::string&) = 0;
};
