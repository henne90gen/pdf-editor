#pragma once

#include <cstring>
#include <string>

#include "md5.h"
#include "sha1.h"

namespace hash {

std::string to_hex_string(const MD5Hash &checksum);
std::string to_hex_string(const SHA1Hash &checksum);

inline uint64_t calculate_padded_size(uint64_t sizeInBytes) {
    if (sizeInBytes % 64 >= 56) {
        return sizeInBytes + 56 + (64 - (sizeInBytes % 64));
    }

    return sizeInBytes + 56 - (sizeInBytes % 64);
}

} // namespace hash
