#pragma once

#include <cstdint>

#include "pdf/memory/arena_allocator.h"
#include "pdf/objects.h"
#include "pdf/util/result.h"

namespace pdf {

struct Image {
    Arena &arena;
    int64_t width            = 0;
    int64_t height           = 0;
    int64_t bitsPerComponent = 0;
    Stream *stream           = nullptr;

    explicit Image(Arena &_arena) : arena(_arena) {}

    /// writes the image to a .bmp file with the given fileName
    [[nodiscard]] Result write_bmp(const std::string &fileName) const;

    /// reads the image file specified by the given file name
    [[nodiscard]] static ValueResult<Image *> read_bmp(Arena &arena, const std::string &fileName);
};

} // namespace pdf
