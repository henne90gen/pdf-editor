#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>

#include "pdf/util/debug.h"
#include "pdf/util/result.h"

namespace pdf {

const size_t ARENA_PAGE_SIZE = 1024 * 1024; // 1 MB

using PtrResult = ValueResult<uint8_t *>;

struct Arena {
    static ValueResult<Arena> create();
    static ValueResult<Arena> create(size_t maximum_size_in_bytes, size_t page_size_in_bytes = ARENA_PAGE_SIZE);

    Arena() = delete;
    explicit Arena(uint8_t *buffer, size_t maximum_size_in_bytes, size_t page_size_in_bytes = ARENA_PAGE_SIZE);
    Arena(Arena &&other);
    Arena &operator=(Arena &&other);
    ~Arena();

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

    uint8_t *current_buffer_position() { return buffer_position; }
    void set_current_buffer_position(uint8_t *position) { buffer_position = position; }

  private:
    uint8_t *buffer_start         = nullptr;
    uint8_t *buffer_position      = nullptr;
    size_t virtual_size_in_bytes  = 0;
    size_t reserved_size_in_bytes = 0;
    size_t page_size_in_bytes     = ARENA_PAGE_SIZE;
};

struct TemporaryAllocator {
    explicit TemporaryAllocator(Arena &_arena)
        : internal_arena(_arena), start_position(_arena.current_buffer_position()) {}
    ~TemporaryAllocator() { internal_arena.set_current_buffer_position(start_position); }

    Arena &arena() { return internal_arena; }

  private:
    Arena &internal_arena;
    uint8_t *start_position = nullptr;
};

struct Allocator {
    static ValueResult<Allocator> create();

    Allocator(Allocator &&other) = default;
    Allocator &operator=(Allocator &&other) {
        internal_arena  = std::move(other.internal_arena);
        temporary_arena = std::move(other.temporary_arena);
        return *this;
    }

    Arena &arena() { return internal_arena; }
    TemporaryAllocator temporary() { return TemporaryAllocator(temporary_arena); }

  private:
    Arena internal_arena;
    Arena temporary_arena;

    explicit Allocator(Arena &_internal_arena, Arena &_temporary_arena)
        : internal_arena(std::move(_internal_arena)), temporary_arena(std::move(_temporary_arena)) {}
};

} // namespace pdf
