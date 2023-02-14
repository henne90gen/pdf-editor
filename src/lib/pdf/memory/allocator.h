#pragma once

#include <cstdlib>
#include <functional>
#include <vector>

#include "pdf/util/debug.h"
#include "pdf/util/result.h"

namespace pdf {

// Forward declaring Object
struct Object;

template <class T, class U>
concept Derived = std::is_base_of<U, T>::value;

struct Allocation {
    char *bufferStart              = nullptr;
    char *bufferPosition           = nullptr;
    size_t sizeInBytes             = 0;
    Allocation *previousAllocation = nullptr;
};

struct Allocator {
    Allocation *currentAllocation      = nullptr;
    std::vector<pdf::Object *> objects = {};

    ~Allocator();
    void init(size_t sizeOfPdfFile);
    void extend(size_t size);
    char *allocate_chunk(size_t sizeInBytes);

    [[nodiscard]] size_t total_bytes_used() const;
    [[nodiscard]] size_t total_bytes_allocated() const;
    [[nodiscard]] size_t num_allocations() const;

    void for_each_allocation(const std::function<ForEachResult(Allocation &)> &func) const;

    template <Derived<pdf::Object> T, typename... Args> T *allocate(Args &&...args) {
        auto s   = sizeof(T);
        auto buf = allocate_chunk(s);
        ASSERT(buf != nullptr);
        T *result = new (buf) T(args...);
        objects.push_back(result);
        return result;
    }

    template <typename T, typename... Args> T *allocate(Args &&...args) {
        auto s   = sizeof(T);
        auto buf = allocate_chunk(s);
        ASSERT(buf != nullptr);
        return new (buf) T(args...);
    }
};

} // namespace pdf
