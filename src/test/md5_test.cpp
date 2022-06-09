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
    const std::string s = "a";

    // WHEN
    auto result = md5::calculate_checksum((uint8_t *)s.c_str(), s.size());

    // THEN
    ASSERT_EQ("0cc175b9c0f1b6a831c399e269772661", md5::to_hex_string(result));
}

TEST(MD5_calculate_checksum, abc) {
    // GIVEN
    const std::string s = "abc";

    // WHEN
    auto result = md5::calculate_checksum((uint8_t *)s.c_str(), s.size());

    // THEN
    ASSERT_EQ("900150983cd24fb0d6963f7d28e17f72", md5::to_hex_string(result));
}

TEST(MD5_calculate_checksum, message_digest) {
    // GIVEN
    const std::string s = "message digest";

    // WHEN
    auto result = md5::calculate_checksum((uint8_t *)s.c_str(), s.size());

    // THEN
    ASSERT_EQ("f96b697d7cb7938d525a2f31aaf161d0", md5::to_hex_string(result));
}

TEST(MD5_calculate_checksum, abcdefghijklmnopqrstuvwxyz) {
    // GIVEN
    const std::string s = "abcdefghijklmnopqrstuvwxyz";

    // WHEN
    auto result = md5::calculate_checksum((uint8_t *)s.c_str(), s.size());

    // THEN
    ASSERT_EQ("c3fcd3d76192e4007dfb496cca67e13b", md5::to_hex_string(result));
}

TEST(MD5_calculate_checksum, ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789) {
    // GIVEN
    const std::string s = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

    // WHEN
    auto result = md5::calculate_checksum((uint8_t *)s.c_str(), s.size());

    // THEN
    ASSERT_EQ("d174ab98d277d9f5a5611c2c9f419d9f", md5::to_hex_string(result));
}

TEST(MD5_calculate_checksum, _12345678901234567890123456789012345678901234567890123456789012345678901234567890) {
    // GIVEN
    const std::string s = "12345678901234567890123456789012345678901234567890123456789012345678901234567890";

    // WHEN
    auto result = md5::calculate_checksum((uint8_t *)s.c_str(), s.size());

    // THEN
    ASSERT_EQ("57edf4a22be3c955ac49da2e2107b67a", md5::to_hex_string(result));
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
    ASSERT_EQ("00102030405060708090a0b0c0d0e0f0", result);
}

TEST(MD5_to_hex_string, SecondNibble) {
    // GIVEN
    md5::MD5Hash hash = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};

    // WHEN
    auto result = md5::to_hex_string(hash);

    // THEN
    ASSERT_EQ("000102030405060708090a0b0c0d0e0f", result);
}
