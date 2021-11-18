#include <gtest/gtest.h>

#include <pdf/helper/allocator.h>
#include <pdf/helper/static_map.h>

TEST(StaticMap, Create) {
    pdf::Allocator allocator = {};
    allocator.init(100);

    std::unordered_map<int, int> map = {{1, 2}, {2, 3}, {3, 4}};
    auto staticMap                   = pdf::StaticMap<int, int>::create(allocator, map);
    auto empty                       = pdf::StaticMap<int, int>::Entry::Status::EMPTY;
    auto filled                      = pdf::StaticMap<int, int>::Entry::Status::FILLED;
    ASSERT_EQ(staticMap.entries[0].status, empty);
    ASSERT_EQ(staticMap.entries[1].status, filled);
    ASSERT_EQ(staticMap.entries[2].status, filled);
    ASSERT_EQ(staticMap.entries[3].status, filled);
    ASSERT_EQ(staticMap.entries[4].status, empty);
    ASSERT_EQ(staticMap.entries[5].status, empty);

    ASSERT_EQ(staticMap.entries[1].key, 1);
    ASSERT_EQ(staticMap.entries[2].key, 2);
    ASSERT_EQ(staticMap.entries[3].key, 3);

    ASSERT_EQ(staticMap.entries[1].value, 2);
    ASSERT_EQ(staticMap.entries[2].value, 3);
    ASSERT_EQ(staticMap.entries[3].value, 4);
}

TEST(StaticMap, Iterate) {
    pdf::Allocator allocator = {};
    allocator.init(100);

    std::unordered_map<int, int> map = {{1, 2}, {2, 3}, {3, 4}};
    auto staticMap                   = pdf::StaticMap<int, int>::create(allocator, map);
    auto itr                         = staticMap.begin();
    ASSERT_EQ(itr->key, 1);
    itr++;
    ASSERT_EQ(itr->key, 2);
    itr++;
    ASSERT_EQ(itr->key, 3);
    itr++;
    ASSERT_EQ(itr, staticMap.end());
}

TEST(StaticMap, Find) {
    pdf::Allocator allocator = {};
    allocator.init(100);

    std::unordered_map<int, int> map = {{1, 2}, {2, 3}, {3, 4}};
    auto staticMap                   = pdf::StaticMap<int, int>::create(allocator, map);
    auto val                         = staticMap.find(1);
    ASSERT_EQ(val, std::optional(2));

    val = staticMap.find(3);
    ASSERT_EQ(val, std::optional(4));

    val = staticMap.find(5);
    ASSERT_EQ(val, std::optional<int>());
}

TEST(StaticMap, Remove) {
    pdf::Allocator allocator = {};
    allocator.init(100);

    std::unordered_map<int, int> map = {{1, 2}, {2, 3}, {3, 4}};
    auto staticMap                   = pdf::StaticMap<int, int>::create(allocator, map);
    auto val                         = staticMap.remove(1);
    ASSERT_EQ(val, std::optional(2));

    val = staticMap.find(1);
    ASSERT_EQ(val, std::optional<int>());
}
