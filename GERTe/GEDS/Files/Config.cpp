#include "Config.h"
#include "../Util/string_utils.h"
#include "../Util/logging.h"
#include "../Util/Error.h"
#include <algorithm>
#include <fstream>
#include <locale>

#ifdef WIN32
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#endif

Config::Config(const std::string& file) {
    bool err = false;

    std::ifstream fstr{ file };

    if (!fstr) {
        error("Failed to open config file " + file);
        return;
    }

    while (!fstr.eof()) {
        std::string line;
        std::getline(fstr, line);

        if (!fstr)
            break;

        auto loc = fstr.getloc();
        std::transform(line.begin(), line.end(), line.begin(), [&loc](auto c){ return std::tolower(c, loc); });

        auto parts = split(line, " ");

        if (parts[0] == "peer") {
            uint16_t port = std::stoul(parts[2]);

            uint32_t ip4;
            if (inet_pton(AF_INET, parts[1].c_str(), &ip4) == 1)
                peers4.emplace_back(ip4, port);
            else {
                ipv6 ip6{};
                if (inet_pton(AF_INET6, parts[1].c_str(), &ip6))
                    peers6.emplace_back(ip6, port);
                else
                    err = true;
            }
        }
    }

    if (err)
        error("Encountered error while parsing config file. Problematic lines omitted.");
}

std::string Config::stringify(uint32_t ip) {
    char cstr[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &ip, cstr, sizeof(cstr)) == nullptr)
        throw std::runtime_error{ generalError("Failed to convert address: ") };

    return std::string{ cstr };
}

std::string Config::stringify(ipv6 ip) {
    char cstr[INET6_ADDRSTRLEN];
    if (inet_ntop(AF_INET6, &ip, cstr, sizeof(cstr)) == nullptr)
        throw std::runtime_error{ generalError("Failed to convert address: ") };

    return std::string{ cstr };
}
