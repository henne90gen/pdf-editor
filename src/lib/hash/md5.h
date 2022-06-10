#pragma once

#include <array>
#include <string>

namespace hash {

typedef std::array<uint32_t, 4> MD5Hash;

MD5Hash md5_checksum(const std::string &str);
MD5Hash md5_checksum(const uint8_t *bytes, uint64_t siteInBytes);

} // namespace hash
