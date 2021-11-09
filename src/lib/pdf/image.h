#pragma once

#include <cstdint>

#include "objects.h"

namespace pdf {

struct Image {
    int64_t width            = 0;
    int64_t height           = 0;
    int64_t bitsPerComponent = 0;
    Stream *stream           = nullptr;

    bool write_bmp(const std::string &fileName) const;
};

} // namespace pdf
