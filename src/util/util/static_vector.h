#pragma once

#include <cstring>
#include <vector>

#include "allocator.h"

namespace util {

template <typename T> struct StaticVector {
    T *elements;
    size_t count;

    T &operator[](size_t index) { return elements[index]; }
    const T &operator[](size_t index) const { return elements[index]; }

    T remove(size_t index) {
        auto result = elements[index];
        for (size_t i = index; i < count - 1; i++) {
            elements[i] = elements[i + 1];
        }
        count--;
        return result;
    }

    [[nodiscard]] size_t size() const { return count; }
    T *begin() { return &elements[0]; }
    T *end() { return &elements[count]; } // 'count' is out of bounds

    static StaticVector<T> create(Allocator &allocator, const std::vector<T> &vec) {
        auto sizeInBytes = vec.size() * sizeof(T);
        auto elements    = (T *)allocator.allocate_chunk(sizeInBytes);
        memcpy(elements, vec.data(), sizeInBytes);
        return StaticVector(elements, vec.size());
    }

  private:
    StaticVector(T *_elements, size_t _count) : elements(_elements), count(_count) {}
};

} // namespace pdf
