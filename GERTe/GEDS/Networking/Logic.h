#pragma once

#include "DataPacket.h"

class Logic {
public:
    virtual void process(DataPacket) = 0;
};
