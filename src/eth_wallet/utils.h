#ifndef UTILS_H_
#define UTILS_H_

#include <string>
#include <vector>

inline bool isDecimal(const std::string &str) {
    if (str.size() >= 2) {
        if (str.substr(0, 2) == "0x") {
            return false;
        }
    }
    for (const char c: str) {
        if ('0' <= c && c <= '9') {
            // ok
        } else {
            return false;
        }
    }
    return true;
}

inline bool isHex(const std::string &str) {
    if (str.size() < 2) {
        return false;
    }
    if (str.substr(0, 2) != "0x") {
        return false;
    }
    for (const char c: str.substr(2)) {
        if (('0' <= c && c <= '9') || ('a' <= c && c <= 'f') || ('A' <= c && c <= 'F')) {
            // ok
        } else {
            return false;
        }
    }
    return true;
}

inline std::string toHex(const std::string &vec) {
    std::string result;
    result.reserve(vec.size() * 2);
    for (const unsigned char c: vec) {
        const unsigned char mod = c % 16;
        const unsigned char div = c / 16;
        result += div <= 9 ? div + '0' : div - 10 + 'a';
        result += mod <= 9 ? mod + '0' : mod - 10 + 'a';
    }
    return result;
}

inline std::string fromHex(const std::string &hex) {
    size_t begin = 0;
    if (hex.compare(0, 2, "0x") == 0) {
        begin = 2;
    }
    std::string result;
    if (hex.size() % 2 != 0) {
        return result;
    }
    result.reserve(hex.size() / 2);
    for (size_t i = begin; i < hex.size(); i += 2) {
        unsigned char mod = hex[i + 1];
        mod = ('0' <= mod && mod <= '9' ? mod - '0' : ('a' <= mod && mod <= 'f' ? mod - 'a' + 10 : mod - 'A' + 10));
        unsigned char div = hex[i];
        div = ('0' <= div && div <= '9' ? div - '0' : ('a' <= div && div <= 'f' ? div - 'a' + 10 : div - 'A' + 10));
        result.push_back(div * 16 + mod);
    }
    return result;
}

#endif // UTILS_H_
