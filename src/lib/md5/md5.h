#include <array>
#include <stddef.h>
#include <stdint.h>
#include <string>

namespace md5 {

typedef std::array<uint8_t, 16> MD5Hash;

MD5Hash calculate_checksum(const uint8_t *bytes, size_t siteInBytes);
std::string to_hex_string(const MD5Hash &checksum);

} // namespace md5
