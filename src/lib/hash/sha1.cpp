#include "sha1.h"

namespace hash {

SHA1Hash sha1_checksum(const std::string &str) { return sha1_checksum((uint8_t *)str.c_str(), str.size()); }

SHA1Hash sha1_checksum(const uint8_t * /*bytes*/, uint64_t /*siteInBytes*/) { return {}; }

} // namespace hash
