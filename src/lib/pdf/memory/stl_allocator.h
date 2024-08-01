#pragma once

#include <cstdlib>
#include <iostream>
#include <limits>
#include <new>
#include <vector>

#include "arena_allocator.h"

template <class T> struct StlAllocator {
    typedef T value_type;

    pdf::Arena &arena;

    StlAllocator(pdf::Arena &arena_) : arena(arena_) {}
    StlAllocator(pdf::Allocator &allocator) : arena(allocator.arena()) {}
    StlAllocator(pdf::TemporaryAllocator &allocator) : arena(allocator.arena()) {}
    template <class U> constexpr StlAllocator(const StlAllocator<U> &other) noexcept : arena(other.arena) {}

    [[nodiscard]] T *allocate(std::size_t n) {
        const auto p = reinterpret_cast<T *>(arena.push(n * sizeof(T)));
        report(p, n);
        return p;
    }

    void deallocate(T *p, std::size_t n) noexcept { report(p, n, false); }

  private:
    void report(T *p, std::size_t n, bool alloc = true) const {
        spdlog::trace("{}: {} bytes at {:#x}", alloc ? "Alloc" : "Dealloc", sizeof(T) * n, reinterpret_cast<uintptr_t>(p));
    }
};

template <class T, class U> bool operator==(const StlAllocator<T> &, const StlAllocator<U> &) {
    // TODO implement this?
    return true;
}
template <class T, class U> bool operator!=(const StlAllocator<T> &, const StlAllocator<U> &) {
    // TODO implement this?
    return false;
}
