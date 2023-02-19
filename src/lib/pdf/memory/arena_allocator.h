#pragma once

#include <cstddef>
#include <cstdint>

const int PAGE_SIZE = 1024 * 1024; // 1 MB

namespace pdf {

struct Arena {
    Arena();
    ~Arena();

    /// push a new allocation onto the stack
    uint8_t *push(const size_t allocationSizeInBytes);
    /// pop an allocation from the stack
    void pop(const size_t allocationSizeInBytes);

    /// allocates a new object and calls its constructor with the provided arguments
    template <typename T, typename... Args> T *push(Args &&...args) {
        auto s   = sizeof(T);
        auto buf = push(s);
        return new (buf) T(args...);
    }

  private:
    uint8_t *bufferStart       = nullptr;
    uint8_t *bufferPosition    = nullptr;
    size_t virtualSizeInBytes  = 0;
    size_t reservedSizeInBytes = 0;
};

} // namespace pdf
