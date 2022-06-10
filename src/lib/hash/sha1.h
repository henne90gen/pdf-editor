#pragma once

#include <array>
#include <string>

namespace hash {

typedef std::array<uint32_t, 5> SHA1Hash;

SHA1Hash sha1_checksum(const std::string &str);
SHA1Hash sha1_checksum(const uint8_t *bytes, uint64_t siteInBytes);

}
