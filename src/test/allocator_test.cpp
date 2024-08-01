#include <gtest/gtest.h>

#include <pdf/memory/arena_allocator.h>
#include <pdf/util/types.h>

// used to make internal fields of Arena accessible during test
struct TestArena {
    uint8_t *buffer_start         = nullptr;
    uint8_t *buffer_position      = nullptr;
    size_t virtual_size_in_bytes  = 0;
    size_t reserved_size_in_bytes = 0;
    size_t page_size_in_bytes     = 0;
};

TEST(Arena, can_handle_small_allocation) {
    ASSERT_EQ(sizeof(TestArena), sizeof(pdf::Arena));

    auto result = pdf::Arena::create();
    ASSERT_FALSE(result.has_error()) << result.message();

    auto &arena    = result.value();
    const auto buf = arena.push(5);
    ASSERT_TRUE(nullptr != buf);
    buf[0] = 0;
    buf[1] = 0;
    buf[2] = 0;
    buf[3] = 0;
    buf[4] = 0;

    const TestArena *testArena = (TestArena *)&arena;
    ASSERT_TRUE(nullptr != testArena->buffer_position);
    ASSERT_TRUE(nullptr != testArena->buffer_start);
    ASSERT_EQ(testArena->buffer_start + 5, testArena->buffer_position);
    constexpr size_t MB = 1024 * 1024;
    constexpr size_t GB = 1024 * MB;
    ASSERT_EQ(128 * GB, testArena->virtual_size_in_bytes);
    ASSERT_EQ(1 * MB, testArena->reserved_size_in_bytes);
}

TEST(Arena, can_handle_growing_allocation) {
    ASSERT_EQ(sizeof(TestArena), sizeof(pdf::Arena));

    auto result = pdf::Arena::create();
    ASSERT_FALSE(result.has_error()) << result.message();

    auto &arena     = result.value();
    const auto buf1 = arena.push(pdf::ARENA_PAGE_SIZE);
    ASSERT_TRUE(nullptr != buf1);

    const auto buf2 = arena.push(1);
    ASSERT_TRUE(nullptr != buf2);

    // returned allocations have adjacent addresses
    ASSERT_EQ(buf1 + pdf::ARENA_PAGE_SIZE, buf2);

    const TestArena *testArena = (TestArena *)&arena;
    ASSERT_TRUE(nullptr != testArena->buffer_position);
    ASSERT_TRUE(nullptr != testArena->buffer_start);
    ASSERT_EQ(testArena->buffer_start + pdf::ARENA_PAGE_SIZE + 1, testArena->buffer_position);
    constexpr size_t MB = 1024 * 1024;
    constexpr size_t GB = 1024 * MB;
    ASSERT_EQ(128 * GB, testArena->virtual_size_in_bytes);
    ASSERT_EQ(2 * MB, testArena->reserved_size_in_bytes);
}

namespace pdf {
pdf::PtrResult ReserveAddressRange(size_t sizeInBytes);
pdf::Result ReleaseAddressRange(uint8_t *buffer, size_t sizeInBytes);
pdf::Result ReserveMemory(uint8_t *buffer, size_t sizeInBytes);
} // namespace pdf

TEST(MemoryApi, ReserveMemory) {
    const size_t GB                   = 1024 * 1024 * 1024;
    const auto reserved_size_in_bytes = 128 * GB;
    auto result                       = pdf::ReserveAddressRange(reserved_size_in_bytes);
    ASSERT_FALSE(result.has_error()) << result.message();
    const auto ptr                  = result.value();
    const auto allocatedSizeInBytes = pdf::ARENA_PAGE_SIZE;
    const auto reserveResult        = pdf::ReserveMemory(ptr, allocatedSizeInBytes);
    ASSERT_FALSE(reserveResult.has_error()) << reserveResult.message();
}

TEST(StlAllocator, CreateVector) {
    auto result = pdf::Arena::create();
    ASSERT_FALSE(result.has_error()) << result.message();
    auto v = pdf::Vector<int>(result.value());
    v.push_back(1);
    v.push_back(2);
}
