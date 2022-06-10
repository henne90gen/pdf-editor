#include "util.h"

namespace hash {

std::string to_hex_string(const MD5Hash &hash) {
    std::string result = "00000000000000000000000000000000";

    auto ptr = (uint8_t *)hash.data();
    for (size_t i = 0; i < 16; i++) {
        char b1 = '0' + (ptr[i] >> 4);
        if (b1 > '9') {
            b1 += 39;
        }
        result[i * 2] = b1;

        char b2 = '0' + (ptr[i] & 0x0f);
        if (b2 > '9') {
            b2 += 39;
        }
        result[i * 2 + 1] = b2;
    }

    return result;
}


std::string to_hex_string(const SHA1Hash &hash) {
    std::string result = "0000000000000000000000000000000000000000";

    auto ptr = (uint8_t *)hash.data();
    for (size_t i = 0; i < 20; i++) {
        char b1 = '0' + (ptr[i] >> 4);
        if (b1 > '9') {
            b1 += 39;
        }
        result[i * 2] = b1;

        char b2 = '0' + (ptr[i] & 0x0f);
        if (b2 > '9') {
            b2 += 39;
        }
        result[i * 2 + 1] = b2;
    }

    return result;
}

} // namespace hash
