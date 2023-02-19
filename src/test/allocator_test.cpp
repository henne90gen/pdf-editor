#include <gtest/gtest.h>

#include <pdf/memory/arena_allocator.h>

// used to make internal fields of Arena accessible during test
struct TestArena {
    uint8_t *bufferStart       = nullptr;
    uint8_t *bufferPosition    = nullptr;
    size_t virtualSizeInBytes  = 0;
    size_t reservedSizeInBytes = 0;
};

TEST(Allocator, arena) {
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
    constexpr size_t MB = 1024 * 1024;
    constexpr size_t GB = 1024 * MB;
    ASSERT_EQ(64 * GB, testArena->virtualSizeInBytes);
    ASSERT_EQ(1 * MB, testArena->reservedSizeInBytes);
}
