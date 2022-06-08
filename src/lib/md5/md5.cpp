#include "md5.h"

#include <cstring>
#include <iostream>

namespace md5 {

std::pair<uint8_t *, size_t> apply_padding(const uint8_t *bytesIn, size_t sizeInBytesIn) {
    size_t sizeInBytesPadded = 0;
    if (sizeInBytesIn % 64 >= 56) {
        sizeInBytesPadded = sizeInBytesIn + 56 + (56 - (sizeInBytesIn % 64));
    } else {
        sizeInBytesPadded = sizeInBytesIn + 56 - (sizeInBytesIn % 64);
    }
    if (sizeInBytesPadded % 64 != 56) {
        std::cout << "wrong padding: " << sizeInBytesPadded % 64 << std::endl;
    }

    uint8_t *bytes = (uint8_t *)malloc(sizeInBytesPadded);
    std::memcpy(bytes, bytesIn, sizeInBytesIn);
    bytes[sizeInBytesIn] = 1;
    for (size_t i = sizeInBytesIn + 1; i < sizeInBytesPadded - sizeInBytesIn; i++) {
        bytes[i] = 0;
    }

    return {bytes, sizeInBytesPadded};
}

MD5Hash calculate_checksum(const uint8_t *bytesIn, size_t sizeInBytesIn) {
    MD5Hash result = {};

    //auto [bytes, sizeInBytes] =
     apply_padding(bytesIn, sizeInBytesIn);

    return result;
}

std::string to_hex_string(const MD5Hash &hash) {
    std::string result = "00000000000000000000000000000000";
    for (size_t i = 0; i < hash.size(); i++) {
        char b1 = '0' + (hash[i] >> 4);
        if (b1 > '9') {
            b1 += 7;
        }
        result[i * 2] = b1;

        char b2 = '0' + (hash[i] & 0x0f);
        if (b2 > '9') {
            b2 += 7;
        }
        result[i * 2 + 1] = b2;
    }
    return result;
}

} // namespace md5
