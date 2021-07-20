#include "string_utils.h"

std::vector<std::string> split(const std::string& source, const std::string& delim) {
    std::vector<std::string> out;

    size_t pos = source.find(delim);
    size_t lpos = 0;

    while (pos != std::string::npos) {
        auto part = source.substr(lpos, pos - lpos);

        if (!part.empty())
            out.push_back(part);

        lpos = pos + 1;
        pos = source.find(delim, lpos);
    }

    auto last = source.substr(lpos);
    if (!last.empty())
        out.push_back(last);

    out.shrink_to_fit();
    return out;
}
