#include <gtest/gtest.h>

#include <pdf/helper/allocator.h>
#include <pdf/helper/static_map.h>

TEST(StaticMap, Create) {
    pdf::Allocator allocator = {};
    allocator.init(100);

    std::unordered_map<int, int> map = {{1, 2}, {2, 3}, {3, 4}};
    auto staticMap                   = pdf::StaticMap<int, int>::create(allocator, map);
//    ASSERT_EQ(staticMap[1], 2);
//    ASSERT_EQ(staticMap[2], 3);
//    ASSERT_EQ(staticMap[3], 4);
}

TEST(StaticMap, Find) {
    pdf::Allocator allocator = {};
    allocator.init(100);

    std::unordered_map<int, int> map = {{1, 2}, {2, 3}, {3, 4}};
    auto staticMap                   = pdf::StaticMap<int, int>::create(allocator, map);
}
