#pragma once

#include <cstdint>

#include "objects.h"
#include "util/allocator.h"

namespace pdf {

struct Image {
    Allocator &allocator;
    int64_t width            = 0;
    int64_t height           = 0;
    int64_t bitsPerComponent = 0;
    Stream *stream           = nullptr;

    /// writes the image to a .bmp file with the given fileName, returns false on success
    [[nodiscard]] Result write_bmp(const std::string &fileName) const;
};

} // namespace pdf
