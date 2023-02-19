#pragma once

#include <cstddef>
#include <cstdint>
#include <sys/mman.h>

#include "pdf/util/debug.h"

namespace pdf {

const size_t ARENA_PAGE_SIZE = 1024 * 1024; // 1 MB

struct Arena {
    Arena();
    explicit Arena(size_t maximumSizeInBytes, size_t pageSize = ARENA_PAGE_SIZE);
    ~Arena();

    /// push a new allocation onto the stack
    uint8_t *push(size_t allocationSizeInBytes);
    /// pop an allocation from the stack
    void pop(size_t allocationSizeInBytes);

    /// allocates a new object and calls its constructor with the provided arguments
    template <typename T, typename... Args> T *push(Args &&...args) {
        auto s   = sizeof(T);
        auto buf = push(s);
        return new (buf) T(args...);
    }

    /// pops the allocation for an object from the stack
    template <typename T> void pop() {
        auto s = sizeof(T);
        pop(s);
    }

  private:
    uint8_t *bufferStart       = nullptr;
    uint8_t *bufferPosition    = nullptr;
    size_t virtualSizeInBytes  = 0;
    size_t reservedSizeInBytes = 0;
    size_t pageSize            = ARENA_PAGE_SIZE;

    void init(size_t maximumSizeInBytes);
};

} // namespace pdf
