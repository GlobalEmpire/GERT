#include "DataPacket.h"

#ifdef WIN32
#include <winsock.h>
#else
#include <arpa/inet.h>
#endif

int DataPacket::needed() const {
    return (raw.capacity() - raw.length()) - partial.length() + 32;
}

bool DataPacket::parse(const std::string& newStr) {
    partial += newStr;

    if (raw.empty()) {
        if (partial.length() < 2)
            return false;
        else {
            std::string len = partial.substr(0, 2);
            partial.erase(0, 2);

            short length = ntohs(*(short*)len.c_str());
            raw.reserve(length + 14);
            data.reserve(length);
        }
    }

    if (raw.length() < 14) {
        if (partial.length() < 14)
            return false;
        else {
            source = GERTc { partial.substr(0, 6) };
            destination = GERTc { partial.substr(6, 6) };
            raw += partial.substr(0, 12);
            partial.erase(0, 12);
        }
    }

    if (data.length() < data.capacity()) {
        int diff = data.capacity() - data.length();
        data += partial.substr(0, diff);
        data.erase(0, diff);

        if (data.length() != data.capacity())
            return false;
    }

    if (partial.length() == 32) {
        hmac = partial;
        partial.clear();
        partial.shrink_to_fit();
        return true;
    }

    return false;
}
