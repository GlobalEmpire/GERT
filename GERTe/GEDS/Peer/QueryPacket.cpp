#include "QueryPacket.h"

QueryPacket::QueryPacket(): CommandPacket(0x00, 0) {
    raw.reserve(3);
}

bool QueryPacket::parse(const std::string& data) {
    partial += data;

    if (raw.empty()) {
        if (partial.length() < 3)
            return false;

        query = Address(partial);
        raw += partial.substr(0, 3);
        partial.clear();
    }

    return true;
}
