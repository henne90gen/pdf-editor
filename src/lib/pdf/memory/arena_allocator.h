#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>

#include "pdf/util/debug.h"
#include "pdf/util/result.h"

namespace pdf {

const size_t ARENA_PAGE_SIZE = 1024 * 1024; // 1 MB

typedef ValueResult<uint8_t *> PtrResult;

struct Arena {
    static ValueResult<Arena> create();
    static ValueResult<Arena> create(size_t maximumSizeInBytes, size_t pageSize = ARENA_PAGE_SIZE);

    Arena() = default;
    explicit Arena(uint8_t *_buffer, size_t _maximumSizeInBytes, size_t _pageSize = ARENA_PAGE_SIZE);

    /// push a new allocation into the arena
    uint8_t *push(size_t allocationSizeInBytes);
    /// pop an allocation from the arena
    void pop(size_t allocationSizeInBytes);
    /// pops all allocations from the arena
    void pop_all();

    /// allocates a new object in the arena and calls its constructor with the provided arguments
    template <typename T, typename... Args> T *push(Args &&...args) {
        auto s   = sizeof(T);
        auto buf = push(s);
        return new (buf) T(args...);
    }

    /// pops the allocation for an object from the arena
    template <typename T> void pop() {
        auto s = sizeof(T);
        pop(s);
    }

    /// releases the underlying memory
    void destroy();

  private:
    uint8_t *bufferStart       = nullptr;
    uint8_t *bufferPosition    = nullptr;
    size_t virtualSizeInBytes  = 0;
    size_t reservedSizeInBytes = 0;
    size_t pageSize            = ARENA_PAGE_SIZE;
};

struct TemporaryArena {
    explicit TemporaryArena(Arena &_arena) : internalArena(_arena) {}
    ~TemporaryArena() { internalArena.pop_all(); }

    Arena &arena() { return internalArena; }

  private:
    Arena &internalArena;
};

struct Allocator {
    static ValueResult<Allocator> create();

    Allocator() = default;

    Arena &arena() { return internalArena; }
    TemporaryArena get_temp() { return TemporaryArena(temporaryArena); }

  private:
    Arena internalArena;
    Arena temporaryArena;

    explicit Allocator(Arena _internalArena, Arena _temporaryArena)
        : internalArena(_internalArena), temporaryArena(_temporaryArena) {}
};

} // namespace pdf
