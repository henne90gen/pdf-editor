#include <gtest/gtest.h>

#include <pdf/memory/arena_allocator.h>

// used to make internal fields of Arena accessible during test
struct TestArena {
    uint8_t *bufferStart       = nullptr;
    uint8_t *bufferPosition    = nullptr;
    size_t virtualSizeInBytes  = 0;
    size_t reservedSizeInBytes = 0;
};

TEST(Arena, can_handle_small_allocation) {
    ASSERT_EQ(sizeof(TestArena), sizeof(pdf::Arena));

    pdf::Arena arena = {};
    const auto buf   = arena.push(5);
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
    ASSERT_EQ(64 * GB, testArena->virtualSizeInBytes);
    ASSERT_EQ(1 * MB, testArena->reservedSizeInBytes);
}

TEST(Arena, can_handle_growing_allocation) {
    ASSERT_EQ(sizeof(TestArena), sizeof(pdf::Arena));

    pdf::Arena arena = {};
    const auto buf1  = arena.push(PAGE_SIZE);
    ASSERT_TRUE(nullptr != buf1);

    const auto buf2 = arena.push(1);
    ASSERT_TRUE(nullptr != buf2);

    // returned allocations have adjacent addresses
    ASSERT_EQ(buf1 + PAGE_SIZE, buf2);

    const TestArena *testArena = (TestArena *)&arena;
    ASSERT_TRUE(nullptr != testArena->bufferPosition);
    ASSERT_TRUE(nullptr != testArena->bufferStart);
    ASSERT_EQ(testArena->bufferStart + PAGE_SIZE + 1, testArena->bufferPosition);
    constexpr size_t MB = 1024 * 1024;
    constexpr size_t GB = 1024 * MB;
    ASSERT_EQ(64 * GB, testArena->virtualSizeInBytes);
    ASSERT_EQ(2 * MB, testArena->reservedSizeInBytes);
}
