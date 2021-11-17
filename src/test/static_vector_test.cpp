#include <gtest/gtest.h>

#include <pdf/helper/allocator.h>
#include <pdf/helper/static_vector.h>

TEST(StaticVector, Create) {
    pdf::Allocator allocator = {};
    allocator.init(100);

    std::vector<int> vec = {1, 2, 3};
    auto staticVector    = pdf::StaticVector<int>::create(allocator, vec);
    ASSERT_EQ(staticVector.elements[0], 1);
    ASSERT_EQ(staticVector.elements[1], 2);
    ASSERT_EQ(staticVector.elements[2], 3);

    ASSERT_EQ(staticVector[0], 1);
    ASSERT_EQ(staticVector[1], 2);
    ASSERT_EQ(staticVector[2], 3);
}

TEST(StaticVector, Remove) {
    pdf::Allocator allocator = {};
    allocator.init(100);

    std::vector<int> vec = {1, 2, 3};
    auto staticVector    = pdf::StaticVector<int>::create(allocator, vec);

    auto removed = staticVector.remove(1);
    ASSERT_EQ(removed, 2);

    ASSERT_EQ(staticVector[0], 1);
    ASSERT_EQ(staticVector[1], 3);
}
