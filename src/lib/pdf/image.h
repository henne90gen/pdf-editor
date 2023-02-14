#pragma once

#include <cstdint>

#include "pdf/objects.h"
#include "pdf/memory/allocator.h"

namespace pdf {

struct Image {
    Allocator &allocator;
    int64_t width            = 0;
    int64_t height           = 0;
    int64_t bitsPerComponent = 0;
    Stream *stream           = nullptr;

    explicit Image(Allocator &_allocator) : allocator(_allocator) {}

    /// writes the image to a .bmp file with the given fileName
    [[nodiscard]] Result write_bmp(const std::string &fileName) const;

    /// reads the image file specified by the given file name
    [[nodiscard]] static ValueResult<Image *> read_bmp(Allocator &allocator, const std::string &fileName);
};

} // namespace pdf
