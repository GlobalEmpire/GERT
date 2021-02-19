#pragma once
#include "../Networking/ReentrantPacket.h"

class CommandPacket: public ReentrantPacket {
protected:
    CommandPacket(unsigned char, int);

public:
    unsigned char command ;
};
