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

namespace md5 {
std::pair<uint8_t *, size_t> apply_padding(const uint8_t *bytesIn, size_t sizeInBytesIn);
uint32_t F(uint32_t X, uint32_t Y, uint32_t Z);
uint32_t G(uint32_t X, uint32_t Y, uint32_t Z);
uint32_t H(uint32_t X, uint32_t Y, uint32_t Z);
uint32_t I(uint32_t X, uint32_t Y, uint32_t Z);
} // namespace md5

TEST(MD5_apply_padding, abc) {
    // GIVEN
    const std::string s = "abc";

    // WHEN
    auto [hash, sizeInBytes] = md5::apply_padding((uint8_t *)s.c_str(), s.size());

    // THEN
    ASSERT_EQ(64, sizeInBytes);
    ASSERT_EQ(hash[3], 0b10000000);
    for (int i = 4; i < 56; i++) {
        ASSERT_EQ(hash[i], 0);
    }
    ASSERT_EQ(*(uint64_t *)(hash + 56), s.size() * 8);
}

TEST(MD5_apply_padding, InputWithLength55) {
    // GIVEN
    std::string s;
    for (int i = 0; i < 55; i++) {
        s += "a";
    }

    // WHEN
    auto [hash, sizeInBytes] = md5::apply_padding((uint8_t *)s.c_str(), s.size());

    // THEN
    ASSERT_EQ(64, sizeInBytes);
    ASSERT_EQ(hash[55], 0b10000000);
    ASSERT_EQ(*(uint64_t *)(hash + 56), s.size() * 8);
}

TEST(MD5_apply_padding, InputWithLength56) {
    // GIVEN
    std::string s;
    for (int i = 0; i < 56; i++) {
        s += "a";
    }

    // WHEN
    auto [hash, sizeInBytes] = md5::apply_padding((uint8_t *)s.c_str(), s.size());

    // THEN
    ASSERT_EQ(128, sizeInBytes);
    ASSERT_EQ(hash[56], 0b10000000);
    for (int i = 57; i < 120; i++) {
        ASSERT_EQ(hash[i], 0) << i;
    }
    ASSERT_EQ(*(uint64_t *)(hash + 120), s.size() * 8);
}

// TEST(MD5_auxiliary_functions, F) {
//     ASSERT_EQ(0x10000001, md5::F(0x00000001, 0x00000001, 0x10000000));
//     ASSERT_EQ(0x11110000, md5::F(0x11111111, 0x11110000, 0x00000000));
// }
//
// TEST(MD5_auxiliary_functions, G) {
//     ASSERT_EQ(0x10000001, md5::G(0x10000000, 0x00000001, 0x10000000));
//     ASSERT_EQ(0x11110000, md5::G(0x11111111, 0x11110000, 0x00000000));
// }
//
// TEST(MD5_auxiliary_functions, H) {
//     ASSERT_EQ(0x10000000, md5::H(0x00000001, 0x00000001, 0x10000000));
//     ASSERT_EQ(0x00001111, md5::H(0x11111111, 0x11110000, 0x00000000));
// }
//
// TEST(MD5_auxiliary_functions, I) {
//     ASSERT_EQ(0xeffffffe, md5::I(0x00000001, 0x00000001, 0x10000000));
//     ASSERT_EQ(0xeeeeffff, md5::I(0x11111111, 0x11110000, 0x00000000));
// }

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
