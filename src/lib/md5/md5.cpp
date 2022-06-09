#include "md5.h"

#include <cmath>
#include <cstring>

namespace md5 {

std::pair<uint8_t *, size_t> apply_padding(const uint8_t *bytesIn, uint64_t sizeInBytesIn) {
    size_t sizeInBytesPadded = 0;
    if (sizeInBytesIn % 64 >= 56) {
        sizeInBytesPadded = sizeInBytesIn + 56 + (64 - (sizeInBytesIn % 64));
    } else {
        sizeInBytesPadded = sizeInBytesIn + 56 - (sizeInBytesIn % 64);
    }

    auto bytes = (uint8_t *)malloc(sizeInBytesPadded + 8);
    std::memcpy(bytes, bytesIn, sizeInBytesIn);
    bytes[sizeInBytesIn] = 0b10000000;
    for (size_t i = sizeInBytesIn + 1; i < sizeInBytesPadded; i++) {
        bytes[i] = 0;
    }

    auto ptr            = (uint64_t *)(bytes + sizeInBytesPadded);
    uint64_t sizeInBits = sizeInBytesIn << 3;
    *ptr                = sizeInBits;

    return {bytes, sizeInBytesPadded + 8};
}

static inline uint32_t F(uint32_t X, uint32_t Y, uint32_t Z) { return (X & Y) | (~X & Z); }
static inline uint32_t G(uint32_t X, uint32_t Y, uint32_t Z) { return (X & Z) | (Y & ~Z); }
static inline uint32_t H(uint32_t X, uint32_t Y, uint32_t Z) { return X ^ Y ^ Z; }
static inline uint32_t I(uint32_t X, uint32_t Y, uint32_t Z) { return Y ^ (X | ~Z); }

#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32 - (n))))
#define ROUND1(a, b, c, d, k, s, i) a = (b) + ROTATE_LEFT(((a) + F(b, c, d) + X[k] + T[(i)-1]), (s))
#define ROUND2(a, b, c, d, k, s, i) a = (b) + ROTATE_LEFT(((a) + G(b, c, d) + X[k] + T[(i)-1]), (s))
#define ROUND3(a, b, c, d, k, s, i) a = (b) + ROTATE_LEFT(((a) + H(b, c, d) + X[k] + T[(i)-1]), (s))
#define ROUND4(a, b, c, d, k, s, i) a = (b) + ROTATE_LEFT(((a) + I(b, c, d) + X[k] + T[(i)-1]), (s))

#define S11 7
#define S12 12
#define S13 17
#define S14 22
#define S21 5
#define S22 9
#define S23 14
#define S24 20
#define S31 4
#define S32 11
#define S33 16
#define S34 23
#define S41 6
#define S42 10
#define S43 15
#define S44 21

MD5Hash calculate_checksum(const uint8_t *bytesIn, uint64_t sizeInBytesIn) {
    auto [bytes, sizeInBytes] = apply_padding(bytesIn, sizeInBytesIn);

    uint32_t T[64] = {
          0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee, 0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
          0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be, 0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
          0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa, 0xd62f105d, 0x2441453,  0xd8a1e681, 0xe7d3fbc8,
          0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed, 0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
          0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c, 0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
          0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x4881d05,  0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
          0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039, 0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
          0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1, 0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391,
    };

    uint32_t A = 0x67452301;
    uint32_t B = 0xefcdab89;
    uint32_t C = 0x98badcfe;
    uint32_t D = 0x10325476;

    // process input in 16-word (64 byte) chunks
    for (uint64_t i = 0; i < sizeInBytes / 64; i++) {
        uint32_t X[16];
        std::memcpy(X, bytes + i * 64, 64);

        auto AA = A;
        auto BB = B;
        auto CC = C;
        auto DD = D;

        ROUND1(A, B, C, D, 0, S11, 1);
        ROUND1(D, A, B, C, 1, S12, 2);
        ROUND1(C, D, A, B, 2, S13, 3);
        ROUND1(B, C, D, A, 3, S14, 4);
        ROUND1(A, B, C, D, 4, S11, 5);
        ROUND1(D, A, B, C, 5, S12, 6);
        ROUND1(C, D, A, B, 6, S13, 7);
        ROUND1(B, C, D, A, 7, S14, 8);
        ROUND1(A, B, C, D, 8, S11, 9);
        ROUND1(D, A, B, C, 9, S12, 10);
        ROUND1(C, D, A, B, 10, S13, 11);
        ROUND1(B, C, D, A, 11, S14, 12);
        ROUND1(A, B, C, D, 12, S11, 13);
        ROUND1(D, A, B, C, 13, S12, 14);
        ROUND1(C, D, A, B, 14, S13, 15); // ERROR c has the wrong value after this line!
        ROUND1(B, C, D, A, 15, S14, 16);

        ROUND2(A, B, C, D, 1, S21, 17);
        ROUND2(D, A, B, C, 6, S22, 18);
        ROUND2(C, D, A, B, 11, S23, 19);
        ROUND2(B, C, D, A, 0, S24, 20);
        ROUND2(A, B, C, D, 5, S21, 21);
        ROUND2(D, A, B, C, 10, S22, 22);
        ROUND2(C, D, A, B, 15, S23, 23);
        ROUND2(B, C, D, A, 4, S24, 24);
        ROUND2(A, B, C, D, 9, S21, 25);
        ROUND2(D, A, B, C, 14, S22, 26);
        ROUND2(C, D, A, B, 3, S23, 27);
        ROUND2(B, C, D, A, 8, S24, 28);
        ROUND2(A, B, C, D, 13, S21, 29);
        ROUND2(D, A, B, C, 2, S22, 30);
        ROUND2(C, D, A, B, 7, S23, 31);
        ROUND2(B, C, D, A, 12, S24, 32);

        ROUND3(A, B, C, D, 5, S31, 33);
        ROUND3(D, A, B, C, 8, S32, 34);
        ROUND3(C, D, A, B, 11, S33, 35);
        ROUND3(B, C, D, A, 14, S34, 36);
        ROUND3(A, B, C, D, 1, S31, 37);
        ROUND3(D, A, B, C, 4, S32, 38);
        ROUND3(C, D, A, B, 7, S33, 39);
        ROUND3(B, C, D, A, 10, S34, 40);
        ROUND3(A, B, C, D, 13, S31, 41);
        ROUND3(D, A, B, C, 0, S32, 42);
        ROUND3(C, D, A, B, 3, S33, 43);
        ROUND3(B, C, D, A, 6, S34, 44);
        ROUND3(A, B, C, D, 9, S31, 45);
        ROUND3(D, A, B, C, 12, S32, 46);
        ROUND3(C, D, A, B, 15, S33, 47);
        ROUND3(B, C, D, A, 2, S34, 48);

        ROUND4(A, B, C, D, 0, S41, 49);
        ROUND4(D, A, B, C, 7, S42, 50);
        ROUND4(C, D, A, B, 14, S43, 51);
        ROUND4(B, C, D, A, 5, S44, 52);
        ROUND4(A, B, C, D, 12, S41, 53);
        ROUND4(D, A, B, C, 3, S42, 54);
        ROUND4(C, D, A, B, 10, S43, 55);
        ROUND4(B, C, D, A, 1, S44, 56);
        ROUND4(A, B, C, D, 8, S41, 57);
        ROUND4(D, A, B, C, 15, S42, 58);
        ROUND4(C, D, A, B, 6, S43, 59);
        ROUND4(B, C, D, A, 13, S44, 60);
        ROUND4(A, B, C, D, 4, S41, 61);
        ROUND4(D, A, B, C, 11, S42, 62);
        ROUND4(C, D, A, B, 2, S43, 63);
        ROUND4(B, C, D, A, 9, S44, 64);

        A += AA;
        B += BB;
        C += CC;
        D += DD;
    }

    std::array<uint32_t, 4> input  = {A, B, C, D};
    std::array<uint8_t, 16> output = {};
    for (unsigned int i = 0, j = 0; j < 16; i++, j += 4) {
        output[j]     = (unsigned char)(input[i] & 0xff);
        output[j + 1] = (unsigned char)((input[i] >> 8) & 0xff);
        output[j + 2] = (unsigned char)((input[i] >> 16) & 0xff);
        output[j + 3] = (unsigned char)((input[i] >> 24) & 0xff);
    }

    return output;
}

std::string to_hex_string(const MD5Hash &hash) {
    std::string result = "00000000000000000000000000000000";

    for (size_t i = 0; i < hash.size(); i++) {
        char b1 = '0' + (hash[i] >> 4);
        if (b1 > '9') {
            b1 += 39;
        }
        result[i * 2] = b1;

        char b2 = '0' + (hash[i] & 0x0f);
        if (b2 > '9') {
            b2 += 39;
        }
        result[i * 2 + 1] = b2;
    }

    return result;
}

} // namespace md5
