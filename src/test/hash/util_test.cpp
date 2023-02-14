#include <gtest/gtest.h>

#include <pdf/hash/md5.h>
#include <pdf/hash/hex_string.h>

TEST(MD5_to_hex_string, AllZeros) {
    // GIVEN
    pdf::hash::MD5Hash hash = {0, 0, 0, 0};

    // WHEN
    auto result = pdf::hash::to_hex_string(hash);

    // THEN
    ASSERT_EQ("00000000000000000000000000000000", result);
}

TEST(MD5_to_hex_string, FirstNibble) {
    // GIVEN
    pdf::hash::MD5Hash hash = {0x30201000, 0x70605040, 0xb0a09080, 0xf0e0d0c0};

    // WHEN
    auto result = pdf::hash::to_hex_string(hash);

    // THEN
    ASSERT_EQ("00102030405060708090a0b0c0d0e0f0", result);
}

TEST(MD5_to_hex_string, SecondNibble) {
    // GIVEN
    pdf::hash::MD5Hash hash = {0x03020100, 0x07060504, 0x0b0a0908, 0x0f0e0d0c};

    // WHEN
    auto result = pdf::hash::to_hex_string(hash);

    // THEN
    ASSERT_EQ("000102030405060708090a0b0c0d0e0f", result);
}
