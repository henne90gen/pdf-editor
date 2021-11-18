#pragma once

#include <cstdlib>
#include <functional>
#include <vector>

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

    ~Allocator();
    void init(size_t sizeOfPdfFile);
    void extend(size_t size);
    char *allocate_chunk(size_t sizeInBytes);

    [[nodiscard]] size_t total_bytes_used() const;
    [[nodiscard]] size_t total_bytes_allocated() const;
    [[nodiscard]] size_t num_allocations() const;

    void for_each_allocation(const std::function<bool(Allocation &)> &func) const;

    template <typename T, typename... Args> T *allocate(Args &&...args) {
        auto s   = sizeof(T);
        auto buf = allocate_chunk(s);
        ASSERT(buf != nullptr);
        return new (buf) T(args...);
    }
};

} // namespace pdf
