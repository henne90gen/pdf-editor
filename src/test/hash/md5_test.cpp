#include <gtest/gtest.h>

#include <hash/md5.h>
#include <hash/util.h>

TEST(MD5_checksum, EmptyString) {
    // GIVEN
    const std::string s = "";

    // WHEN
    auto result = hash::md5_checksum(s);

    // THEN
    ASSERT_EQ("d41d8cd98f00b204e9800998ecf8427e", hash::to_hex_string(result));
}

TEST(MD5_checksum, a) {
    // GIVEN
    const std::string s = "a";

    // WHEN
    auto result = hash::md5_checksum(s);

    // THEN
    ASSERT_EQ("0cc175b9c0f1b6a831c399e269772661", hash::to_hex_string(result));
}

TEST(MD5_checksum, abc) {
    // GIVEN
    const std::string s = "abc";

    // WHEN
    auto result = hash::md5_checksum(s);

    // THEN
    ASSERT_EQ("900150983cd24fb0d6963f7d28e17f72", hash::to_hex_string(result));
}

TEST(MD5_checksum, message_digest) {
    // GIVEN
    const std::string s = "message digest";

    // WHEN
    auto result = hash::md5_checksum(s);

    // THEN
    ASSERT_EQ("f96b697d7cb7938d525a2f31aaf161d0", hash::to_hex_string(result));
}

TEST(MD5_checksum, abcdefghijklmnopqrstuvwxyz) {
    // GIVEN
    const std::string s = "abcdefghijklmnopqrstuvwxyz";

    // WHEN
    auto result = hash::md5_checksum(s);

    // THEN
    ASSERT_EQ("c3fcd3d76192e4007dfb496cca67e13b", hash::to_hex_string(result));
}

TEST(MD5_checksum, ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789) {
    // GIVEN
    const std::string s = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

    // WHEN
    auto result = hash::md5_checksum(s);

    // THEN
    ASSERT_EQ("d174ab98d277d9f5a5611c2c9f419d9f", hash::to_hex_string(result));
}

TEST(MD5_checksum, _12345678901234567890123456789012345678901234567890123456789012345678901234567890) {
    // GIVEN
    const std::string s = "12345678901234567890123456789012345678901234567890123456789012345678901234567890";

    // WHEN
    auto result = hash::md5_checksum(s);

    // THEN
    ASSERT_EQ("57edf4a22be3c955ac49da2e2107b67a", hash::to_hex_string(result));
}

TEST(MD5_checksum, Length63) {
    // GIVEN
    const std::string s = "abcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabc";

    // WHEN
    auto result = hash::md5_checksum(s);

    // THEN
    ASSERT_EQ("25982a1d97e97318d387f77aa0252ac4", hash::to_hex_string(result));
}

TEST(MD5_checksum, Length64) {
    // GIVEN
    const std::string s = "abcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabca";

    // WHEN
    auto result = hash::md5_checksum(s);

    // THEN
    ASSERT_EQ("efefeedae197ed614b582e1490f9e33e", hash::to_hex_string(result));
}

TEST(MD5_checksum, Length65) {
    // GIVEN
    const std::string s = "abcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcab";

    // WHEN
    auto result = hash::md5_checksum(s);

    // THEN
    ASSERT_EQ("017d572b274249897ce1c9234c92711e", hash::to_hex_string(result));
}
