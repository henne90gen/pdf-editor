#pragma once

#include <array>
#include <string>

namespace hash {

// https://datatracker.ietf.org/doc/html/rfc3174
// https://datatracker.ietf.org/doc/html/rfc6234

typedef std::array<uint32_t, 5> SHA1Hash;

SHA1Hash sha1_checksum(const std::string &str);
SHA1Hash sha1_checksum(const uint8_t *bytes, uint64_t sizeInBytes);

} // namespace hash
