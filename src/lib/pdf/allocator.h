#pragma once

#include <cstdlib>

#include "util.h"

namespace pdf {

struct Allocation {
    char *bufferStart              = nullptr;
    char *bufferPosition           = nullptr;
    size_t sizeInBytes             = 0;
    Allocation *previousAllocation = nullptr;
};

struct Allocator {
    Allocation *currentAllocation = nullptr;

    void init(size_t sizeOfPdfFile);
    char *allocate_chunk(size_t size);

    template <typename T, typename... Args> T *allocate(Args &&...args) {
        auto s   = sizeof(T);
        auto buf = allocate_chunk(s);
        ASSERT(buf != nullptr);
        return new (buf) T(args...);
    }
};

} // namespace pdf
