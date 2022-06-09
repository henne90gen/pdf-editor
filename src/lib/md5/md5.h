#include <array>
#include <string>

namespace md5 {

typedef std::array<uint8_t, 16> MD5Hash;

MD5Hash calculate_checksum(const uint8_t *bytes, uint64_t siteInBytes);
std::string to_hex_string(const MD5Hash &checksum);

} // namespace md5
