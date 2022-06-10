#pragma once

#include <string>

#include "md5.h"
#include "sha1.h"

namespace hash {

std::string to_hex_string(const MD5Hash &checksum);
std::string to_hex_string(const SHA1Hash &checksum);

}
