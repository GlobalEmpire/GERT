#include "QueryPPacket.h"

QueryPPacket::QueryPPacket(): CommandPacket(0x01, 0) {
    raw.reserve(6);
}

bool QueryPPacket::parse(const std::string& data) {
    partial += data;

    if (raw.empty()) {
        if (partial.length() < 3)
            return false;

        query = Address(partial);
        raw += partial.substr(0, 3);
        partial.clear();
    }

    if (raw.length() < 4) {
        if (partial.empty())
            return false;

        major = partial[0];
        raw += partial.substr(0, 1);
        partial.erase(0, 1);
    }

    if (raw.length() < 5) {
        if (partial.empty())
            return false;

        minor = partial[0];
        raw += partial.substr(0, 1);
        partial.erase(0, 1);
    }

    if (raw.length() < raw.capacity()) {
        if (partial.empty())
            return false;

        direct = partial[0] != 0;
        raw += partial.substr(0, 1);
        partial.clear();
    }

    return true;
}
