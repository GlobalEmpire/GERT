#pragma once
#include "../Networking/ReentrantPacket.h"

class CommandPacket: public ReentrantPacket {
protected:
    CommandPacket(unsigned char);

public:
    unsigned char command;
};
