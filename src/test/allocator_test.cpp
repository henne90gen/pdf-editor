#include <gtest/gtest.h>

#include <pdf/memory/arena_allocator.h>

// used to make internal fields of Arena accessible during test
struct TestArena {
    uint8_t *bufferStart       = nullptr;
    uint8_t *bufferPosition    = nullptr;
    size_t virtualSizeInBytes  = 0;
    size_t reservedSizeInBytes = 0;
    size_t pageSize            = 0;
};

TEST(Arena, can_handle_small_allocation) {
    ASSERT_EQ(sizeof(TestArena), sizeof(pdf::Arena));

    auto result = pdf::Arena::create();
    ASSERT_FALSE(result.has_error()) << result.message();

    auto arena     = result.value();
    const auto buf = arena.push(5);
    ASSERT_TRUE(nullptr != buf);
    buf[0] = 0;
    buf[1] = 0;
    buf[2] = 0;
    buf[3] = 0;
    buf[4] = 0;

    const TestArena *testArena = (TestArena *)&arena;
    ASSERT_TRUE(nullptr != testArena->bufferPosition);
    ASSERT_TRUE(nullptr != testArena->bufferStart);
    ASSERT_EQ(testArena->bufferStart + 5, testArena->bufferPosition);
    constexpr size_t MB = 1024 * 1024;
    constexpr size_t GB = 1024 * MB;
    ASSERT_EQ(128 * GB, testArena->virtualSizeInBytes);
    ASSERT_EQ(1 * MB, testArena->reservedSizeInBytes);
}

TEST(Arena, can_handle_growing_allocation) {
    ASSERT_EQ(sizeof(TestArena), sizeof(pdf::Arena));

    auto result = pdf::Arena::create();
    ASSERT_FALSE(result.has_error()) << result.message();

    auto arena      = result.value();
    const auto buf1 = arena.push(pdf::ARENA_PAGE_SIZE);
    ASSERT_TRUE(nullptr != buf1);

    const auto buf2 = arena.push(1);
    ASSERT_TRUE(nullptr != buf2);

    // returned allocations have adjacent addresses
    ASSERT_EQ(buf1 + pdf::ARENA_PAGE_SIZE, buf2);

    const TestArena *testArena = (TestArena *)&arena;
    ASSERT_TRUE(nullptr != testArena->bufferPosition);
    ASSERT_TRUE(nullptr != testArena->bufferStart);
    ASSERT_EQ(testArena->bufferStart + pdf::ARENA_PAGE_SIZE + 1, testArena->bufferPosition);
    constexpr size_t MB = 1024 * 1024;
    constexpr size_t GB = 1024 * MB;
    ASSERT_EQ(128 * GB, testArena->virtualSizeInBytes);
    ASSERT_EQ(2 * MB, testArena->reservedSizeInBytes);
}

namespace pdf {
pdf::PtrResult ReserveAddressRange(size_t sizeInBytes);
pdf::Result ReleaseAddressRange(uint8_t *buffer, size_t sizeInBytes);
pdf::Result ReserveMemory(uint8_t *buffer, size_t sizeInBytes);
} // namespace pdf

TEST(MemoryApi, ReserveMemory) {
    const size_t GB                = 1024 * 1024 * 1024;
    const auto reservedSizeInBytes = 128 * GB;
    auto result                    = pdf::ReserveAddressRange(reservedSizeInBytes);
    ASSERT_FALSE(result.has_error()) << result.message();
    const auto ptr                  = result.value();
    const auto allocatedSizeInBytes = pdf::ARENA_PAGE_SIZE;
    const auto reserveResult        = pdf::ReserveMemory(ptr, allocatedSizeInBytes);
    ASSERT_FALSE(reserveResult.has_error()) << reserveResult.message();
}
