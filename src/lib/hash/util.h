#pragma once

#include <cstring>
#include <string>

#include "md5.h"
#include "sha1.h"

namespace hash {

std::string to_hex_string(const MD5Hash &checksum);
std::string to_hex_string(const SHA1Hash &checksum);

inline uint64_t calculate_padded_size(uint64_t sizeInBytesIn) {
    if (sizeInBytesIn % 64 >= 56) {
        return sizeInBytesIn + 56 + (64 - (sizeInBytesIn % 64));
    }

    return sizeInBytesIn + 56 - (sizeInBytesIn % 64);
}

} // namespace hash
