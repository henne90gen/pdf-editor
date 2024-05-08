#pragma once

#include <array>
#include <cstdint>
#include <string>
namespace pdf::hash {

// https://datatracker.ietf.org/doc/html/rfc1321
// https://datatracker.ietf.org/doc/html/rfc6151

typedef std::array<uint32_t, 4> MD5Hash;

MD5Hash md5_checksum(const std::string &str);
MD5Hash md5_checksum(const uint8_t *bytes, uint64_t sizeInBytes);

} // namespace pdf::hash
