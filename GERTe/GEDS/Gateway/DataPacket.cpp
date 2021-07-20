#include "DataPacket.h"
#include "../Util/logging.h"

#ifdef WIN32
#include <winsock2.h>
#else
#define ntohll be64toh

#include <arpa/inet.h>
#include <endian.h>
#endif


DataPacket::DataPacket() {
    need = 23;
}

bool DataPacket::parse(const std::string& newStr) {
    need -= (int)newStr.length();

    partial += newStr;

    if (raw.empty()) {
        if (partial.length() < 2)
            return false;
        else {
            length = ntohs(*(short*)partial.c_str());
            raw.reserve(length + 23);
            data.reserve(length);

            need += length;

            raw += partial.substr(0, 2);
            partial.erase(0, 2);
        }
    }

    if (raw.length() < 14) {
        if (partial.length() < 12)
            return false;
        else {
            source = GERTc { partial.substr(0, 6) };
            destination = GERTc { partial.substr(6, 6) };
            raw += partial.substr(0, 12);
            partial.erase(0, 12);
        }
    }

    if (data.length() < length) {
        size_t diff = length - data.length();
        data += partial.substr(0, diff);
        partial.erase(0, diff);

        if (data.length() != length)
            return false;
        else
            raw += data;
    }

    if (!stamped) {
        if (partial.length() < 8)
            return false;

        timestamp = ntohll(*(uint64_t*)partial.c_str());
        raw += partial.substr(0, 8);
        partial.erase(0, 8);

        stamped = true;
    }

    if (!lengthed) {
        if (partial.empty())
            return false;
        else {
            sigLength = partial[0];
            raw.reserve(length + sigLength + 23);
            signature.reserve(sigLength);

            partial.erase(0, 1);

            need += sigLength;

            lengthed = true;
        }
    }

    if (signature.length() < sigLength) {
        signature += partial;
        partial.clear();

        if (signature.length() != sigLength)
            return false;
    }

    partial.shrink_to_fit();
    return true;
}
