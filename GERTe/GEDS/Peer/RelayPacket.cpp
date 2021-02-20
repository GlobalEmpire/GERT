#include "RelayPacket.h"

#ifdef WIN32
#include <winsock.h>
#else
#include <arpa/inet.h>
#endif


RelayPacket::RelayPacket(): CommandPacket(0x03, 32) {
    raw.reserve(22);
}

bool RelayPacket::parse(const std::string& newStr) {
    partial += newStr;

    if (subraw.empty()) {
        if (partial.length() < 2)
            return false;
        else {
            std::string len = partial.substr(0, 2);
            partial.erase(0, 2);

            short length = ntohs(*(short*)len.c_str());
            subraw.reserve(length + 22);
            data.reserve(length);
        }
    }

    if (subraw.length() < 14) {
        if (partial.length() < 14)
            return false;
        else {
            source = GERTc { partial.substr(0, 6) };
            destination = GERTc { partial.substr(6, 6) };
            subraw += partial.substr(0, 12);
            partial.erase(0, 12);
        }
    }

    if (data.length() < data.capacity()) {
        size_t diff = data.capacity() - data.length();
        data += partial.substr(0, diff);
        partial.erase(0, diff);

        if (data.length() != data.capacity())
            return false;
        else
            subraw += data;
    }

    if (subraw.length() < subraw.capacity()) {
        if (partial.length() < 8)
            return false;

        timestamp = ntohl(*(unsigned long*)partial.c_str());
        subraw += partial.substr(0, 8);
        partial.erase(0, 8);
    }

    if (partial.length() == 32) {
        signature = partial;
        partial.clear();
        partial.shrink_to_fit();

        raw += subraw;
        return true;
    }

    return false;
}

