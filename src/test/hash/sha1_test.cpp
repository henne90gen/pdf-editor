#include <gtest/gtest.h>

#include <hash/sha1.h>
#include <hash/util.h>

TEST(SHA1_checksum, a) {
    // GIVEN
    std::string s;
    for (int i = 0; i < 1000000; i++) {
        s += "a";
    }

    // WHEN
    auto result = hash::sha1_checksum(s);

    // THEN
    ASSERT_EQ("34aa973cd4c4daa4f61eeb2bdbad27316534016f", hash::to_hex_string(result));
}

TEST(SHA1_checksum, abc) {
    // GIVEN
    const std::string s = "abc";

    // WHEN
    auto result = hash::sha1_checksum(s);

    // THEN
    ASSERT_EQ("a9993e364706816aba3e25717850c26c9cd0d89d", hash::to_hex_string(result));
}

TEST(SHA1_checksum, abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq) {
    // GIVEN
    const std::string s = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";

    // WHEN
    auto result = hash::sha1_checksum(s);

    // THEN
    ASSERT_EQ("84983e441c3bd26ebaae4aa1f95129e5e54670f1", hash::to_hex_string(result));
}

TEST(SHA1_checksum, _0123456701234567012345670123456701234567012345670123456701234567) {
    // GIVEN
    std::string s;
    for (int i = 0; i < 10; i++) {
        s += "0123456701234567012345670123456701234567012345670123456701234567";
    }

    // WHEN
    auto result = hash::sha1_checksum(s);

    // THEN
    ASSERT_EQ("dea356a2cddd90c7a7ecedc5ebb563934f460452", hash::to_hex_string(result));
}
