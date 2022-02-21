#pragma once

#include <cstdlib>
#include <functional>
#include <vector>

#include "result.h"
#include "util.h"

namespace util {

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
    void clear_current_allocation() const;
    char *allocate_chunk(size_t sizeInBytes);

    [[nodiscard]] size_t total_bytes_used() const;
    [[nodiscard]] size_t total_bytes_allocated() const;
    [[nodiscard]] size_t num_allocations() const;

    void for_each_allocation(const std::function<util::ForEachResult(Allocation &)> &func) const;

    template <typename T, typename... Args> T *allocate(Args &&...args) {
        auto s   = sizeof(T);
        auto buf = allocate_chunk(s);
        ASSERT(buf != nullptr);
        return new (buf) T(args...);
    }
};

} // namespace util
