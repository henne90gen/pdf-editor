#pragma once

#include <cstdint>

#include "pdf/memory/arena_allocator.h"
#include "pdf/objects.h"
#include "pdf/util/result.h"

namespace pdf {

struct Image {
    int64_t width            = 0;
    int64_t height           = 0;
    int64_t bitsPerComponent = 0;
    Stream *stream           = nullptr;

    /// writes the image to a .bmp file with the given fileName
    [[nodiscard]] Result write_bmp(Allocator &allocator, const std::string &fileName) const;

    /// reads the image file specified by the given file name
    [[nodiscard]] static ValueResult<Image *> read_bmp(Allocator &allocator, const std::string &fileName);
};

} // namespace pdf
