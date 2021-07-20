#include "RelayPacket.h"

RelayPacket::RelayPacket(): CommandPacket(0x03) {
    need = 23;
}

bool RelayPacket::parse(const std::string& newStr) {
    raw += newStr;
    bool result = subpacket.parse(newStr);
    need = subpacket.needed();

    return result;
}

