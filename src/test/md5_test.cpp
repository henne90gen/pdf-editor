#include <gtest/gtest.h>

#include <md5/md5.h>

TEST(MD5_calculate_checksum, EmptyString) {
    // GIVEN
    size_t size = 0;
    auto bytes  = (uint8_t *)malloc(size);

    // WHEN
    auto result = md5::calculate_checksum(bytes, size);

    // THEN
    ASSERT_EQ("d41d8cd98f00b204e9800998ecf8427e", md5::to_hex_string(result));
}

TEST(MD5_calculate_checksum, a) {
    // GIVEN
    const auto s = "a";

    // WHEN
    auto result = md5::calculate_checksum((uint8_t *)s, sizeof(s));

    // THEN
    ASSERT_EQ("0cc175b9c0f1b6a831c399e269772661", md5::to_hex_string(result));
}

TEST(MD5_calculate_checksum, abc) {
    // GIVEN
    const auto s = "abc";

    // WHEN
    auto result = md5::calculate_checksum((uint8_t *)s, sizeof(s));

    // THEN
    ASSERT_EQ("900150983cd24fb0d6963f7d28e17f72", md5::to_hex_string(result));
}

namespace md5 {
std::pair<uint8_t *, size_t> apply_padding(const uint8_t *bytesIn, size_t sizeInBytesIn);
}

TEST(MD5_apply_padding, abc) {
    // GIVEN
    const auto s = "abc";

    // WHEN
    auto [_, sizeInBytes] = md5::apply_padding((uint8_t *)s, sizeof(s));

    // THEN
    ASSERT_EQ(56, sizeInBytes);
}

TEST(MD5_apply_padding, InputWithLength56) {
    // GIVEN
    std::string s = "";
    for (int i = 0; i < 56; i++) {
        s += "a";
    }

    // WHEN
    auto [_, sizeInBytes] = md5::apply_padding((uint8_t *)s.c_str(), s.size());

    // THEN
    ASSERT_EQ(112, sizeInBytes);
}

TEST(MD5_to_hex_string, AllZeros) {
    // GIVEN
    md5::MD5Hash hash = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    // WHEN
    auto result = md5::to_hex_string(hash);

    // THEN
    ASSERT_EQ("00000000000000000000000000000000", result);
}

TEST(MD5_to_hex_string, FirstNibble) {
    // GIVEN
    md5::MD5Hash hash = {0, 16, 32, 48, 64, 80, 96, 112, 128, 144, 160, 176, 192, 208, 224, 240};

    // WHEN
    auto result = md5::to_hex_string(hash);

    // THEN
    ASSERT_EQ("00102030405060708090A0B0C0D0E0F0", result);
}

TEST(MD5_to_hex_string, SecondNibble) {
    // GIVEN
    md5::MD5Hash hash = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};

    // WHEN
    auto result = md5::to_hex_string(hash);

    // THEN
    ASSERT_EQ("000102030405060708090A0B0C0D0E0F", result);
}
