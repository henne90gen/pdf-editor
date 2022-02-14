#include <gtest/gtest.h>

#include "pdf/objects.h"
#include "util/allocator.h"
#include "util/static_map.h"

TEST(StaticMap, Create) {
    util::Allocator allocator = {};
    allocator.init(100);

    std::unordered_map<int, int> map = {{1, 2}, {2, 3}, {3, 4}};
    auto staticMap                   = util::StaticMap<int, int>::create(allocator, map);
    auto empty                       = util::StaticMap<int, int>::Entry::Status::EMPTY;
    auto filled                      = util::StaticMap<int, int>::Entry::Status::FILLED;
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
    util::Allocator allocator = {};
    allocator.init(100);

    std::unordered_map<int, int> map = {{1, 2}, {2, 3}, {3, 4}};
    auto staticMap                   = util::StaticMap<int, int>::create(allocator, map);
    auto itr                         = staticMap.begin();
    ASSERT_EQ(itr->key, 1);
    itr++;
    ASSERT_EQ(itr->key, 2);
    itr++;
    ASSERT_EQ(itr->key, 3);
    itr++;
    ASSERT_EQ(itr, staticMap.end());
}

TEST(StaticMap, IterateMore) {
    util::Allocator allocator = {};
    allocator.init(100);

    auto map              = std::unordered_map<std::string_view, pdf::Object *>();
    map["Hello"]          = new pdf::Integer(123);
    map["World"]          = new pdf::LiteralString("World");
    const auto &staticMap = util::StaticMap<std::string_view, pdf::Object *>::create(allocator, map);
    for (const auto &entry : staticMap) {
        auto filled = util::StaticMap<std::string_view, pdf::Object *>::Entry::Status::FILLED;
        ASSERT_EQ(entry.status, filled);
    }
}

TEST(StaticMap, Find) {
    util::Allocator allocator = {};
    allocator.init(100);

    std::unordered_map<int, int> map = {{1, 2}, {2, 3}, {3, 4}};
    auto staticMap                   = util::StaticMap<int, int>::create(allocator, map);
    auto val                         = staticMap.find(1);
    ASSERT_EQ(val, std::optional(2));

    val = staticMap.find(3);
    ASSERT_EQ(val, std::optional(4));

    val = staticMap.find(5);
    ASSERT_EQ(val, std::optional<int>());
}

TEST(StaticMap, Remove) {
    util::Allocator allocator = {};
    allocator.init(100);

    std::unordered_map<int, int> map = {{1, 2}, {2, 3}, {3, 4}};
    auto staticMap                   = util::StaticMap<int, int>::create(allocator, map);
    auto val                         = staticMap.remove(1);
    ASSERT_EQ(val, std::optional(2));

    val = staticMap.find(1);
    ASSERT_EQ(val, std::optional<int>());
}

TEST(StaticMap, CreateManyTimes) {
    util::Allocator allocator = {};
    allocator.init(1);

    for (int i = 0; i < 1000; i++) {
        std::unordered_map<std::string, int> map = {{"1", 2}, {"2", 3}, {"3", 4}};
        auto staticMap                           = util::StaticMap<std::string, int>::create(allocator, map);
        ASSERT_EQ(staticMap.size(), 3);
    }
}
