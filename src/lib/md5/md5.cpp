#include "md5.h"

#include <cmath>
#include <cstring>

namespace md5 {

static inline uint32_t F(uint32_t X, uint32_t Y, uint32_t Z) { return (X & Y) | (~X & Z); }
static inline uint32_t G(uint32_t X, uint32_t Y, uint32_t Z) { return (X & Z) | (Y & ~Z); }
static inline uint32_t H(uint32_t X, uint32_t Y, uint32_t Z) { return X ^ Y ^ Z; }
static inline uint32_t I(uint32_t X, uint32_t Y, uint32_t Z) { return Y ^ (X | ~Z); }

#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

// NOTE: using an array for T instead of constants is much faster for some reason
#define USE_ARRAY 1
#if USE_ARRAY
#define ROUND1(a, b, c, d, k, s, i) a = (b) + ROTATE_LEFT(((a) + F(b, c, d) + X[k] + T[i]), (s))
#define ROUND2(a, b, c, d, k, s, i) a = (b) + ROTATE_LEFT(((a) + G(b, c, d) + X[k] + T[i]), (s))
#define ROUND3(a, b, c, d, k, s, i) a = (b) + ROTATE_LEFT(((a) + H(b, c, d) + X[k] + T[i]), (s))
#define ROUND4(a, b, c, d, k, s, i) a = (b) + ROTATE_LEFT(((a) + I(b, c, d) + X[k] + T[i]), (s))
#else
#define ROUND1(a, b, c, d, k, s, i) a = (b) + ROTATE_LEFT(((a) + F(b, c, d) + X[k] + (i)), (s))
#define ROUND2(a, b, c, d, k, s, i) a = (b) + ROTATE_LEFT(((a) + G(b, c, d) + X[k] + (i)), (s))
#define ROUND3(a, b, c, d, k, s, i) a = (b) + ROTATE_LEFT(((a) + H(b, c, d) + X[k] + (i)), (s))
#define ROUND4(a, b, c, d, k, s, i) a = (b) + ROTATE_LEFT(((a) + I(b, c, d) + X[k] + (i)), (s))
#endif

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

MD5Hash calculate_checksum(const std::string &str) { return calculate_checksum((uint8_t *)str.c_str(), str.size()); }

#if USE_ARRAY
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
#endif

static inline uint64_t getPaddedSize(uint64_t sizeInBytesIn) {
    if (sizeInBytesIn % 64 >= 56) {
        return sizeInBytesIn + 56 + (64 - (sizeInBytesIn % 64));
    }

    return sizeInBytesIn + 56 - (sizeInBytesIn % 64);
}

MD5Hash calculate_checksum(const uint8_t *bytesIn, uint64_t sizeInBytesIn) {
    uint64_t sizeInBytesPadded = getPaddedSize(sizeInBytesIn);
    uint64_t sizeInBytesFinal  = sizeInBytesPadded + 8;

    uint32_t A = 0x67452301;
    uint32_t B = 0xefcdab89;
    uint32_t C = 0x98badcfe;
    uint32_t D = 0x10325476;

    // process input in 16-word (64 byte) chunks
    for (uint64_t i = 0; i < sizeInBytesFinal / 64; i++) {
        uint32_t X[16];
        auto offset = i * 64;
        std::memcpy(X, bytesIn + offset, std::min((uint64_t)64, sizeInBytesIn - offset));
        if (offset + 64 > sizeInBytesIn) {
            if (offset < sizeInBytesIn) {
                ((uint8_t *)X)[sizeInBytesIn % 64] = 0b10000000;
                for (size_t j = (sizeInBytesIn % 64) + 1;
                     j < std::max(sizeInBytesPadded % 64, sizeInBytesPadded - sizeInBytesIn); j++) {
                    ((uint8_t *)X)[j] = 0;
                }
            } else {
                for (size_t j = 0; j < 56; j++) {
                    ((uint8_t *)X)[j] = 0;
                }
                if (offset == sizeInBytesIn) {
                    ((uint8_t *)X)[sizeInBytesIn % 64] = 0b10000000;
                }
            }

            if (sizeInBytesIn % 64 < 56 || offset > sizeInBytesIn) {
                auto ptr            = (uint64_t *)(((uint8_t *)X) + (sizeInBytesPadded % 64));
                uint64_t sizeInBits = sizeInBytesIn << 3;
                *ptr                = sizeInBits;
            }
        }

        auto AA = A;
        auto BB = B;
        auto CC = C;
        auto DD = D;

#if USE_ARRAY
        ROUND1(A, B, C, D, 0, S11, 0);
        ROUND1(D, A, B, C, 1, S12, 1);
        ROUND1(C, D, A, B, 2, S13, 2);
        ROUND1(B, C, D, A, 3, S14, 3);
        ROUND1(A, B, C, D, 4, S11, 4);
        ROUND1(D, A, B, C, 5, S12, 5);
        ROUND1(C, D, A, B, 6, S13, 6);
        ROUND1(B, C, D, A, 7, S14, 7);
        ROUND1(A, B, C, D, 8, S11, 8);
        ROUND1(D, A, B, C, 9, S12, 9);
        ROUND1(C, D, A, B, 10, S13, 10);
        ROUND1(B, C, D, A, 11, S14, 11);
        ROUND1(A, B, C, D, 12, S11, 12);
        ROUND1(D, A, B, C, 13, S12, 13);
        ROUND1(C, D, A, B, 14, S13, 14);
        ROUND1(B, C, D, A, 15, S14, 15);

        ROUND2(A, B, C, D, 1, S21, 16);
        ROUND2(D, A, B, C, 6, S22, 17);
        ROUND2(C, D, A, B, 11, S23, 18);
        ROUND2(B, C, D, A, 0, S24, 19);
        ROUND2(A, B, C, D, 5, S21, 20);
        ROUND2(D, A, B, C, 10, S22, 21);
        ROUND2(C, D, A, B, 15, S23, 22);
        ROUND2(B, C, D, A, 4, S24, 23);
        ROUND2(A, B, C, D, 9, S21, 24);
        ROUND2(D, A, B, C, 14, S22, 25);
        ROUND2(C, D, A, B, 3, S23, 26);
        ROUND2(B, C, D, A, 8, S24, 27);
        ROUND2(A, B, C, D, 13, S21, 28);
        ROUND2(D, A, B, C, 2, S22, 29);
        ROUND2(C, D, A, B, 7, S23, 30);
        ROUND2(B, C, D, A, 12, S24, 31);

        ROUND3(A, B, C, D, 5, S31, 32);
        ROUND3(D, A, B, C, 8, S32, 33);
        ROUND3(C, D, A, B, 11, S33, 34);
        ROUND3(B, C, D, A, 14, S34, 35);
        ROUND3(A, B, C, D, 1, S31, 36);
        ROUND3(D, A, B, C, 4, S32, 37);
        ROUND3(C, D, A, B, 7, S33, 38);
        ROUND3(B, C, D, A, 10, S34, 39);
        ROUND3(A, B, C, D, 13, S31, 40);
        ROUND3(D, A, B, C, 0, S32, 41);
        ROUND3(C, D, A, B, 3, S33, 42);
        ROUND3(B, C, D, A, 6, S34, 43);
        ROUND3(A, B, C, D, 9, S31, 44);
        ROUND3(D, A, B, C, 12, S32, 45);
        ROUND3(C, D, A, B, 15, S33, 46);
        ROUND3(B, C, D, A, 2, S34, 47);

        ROUND4(A, B, C, D, 0, S41, 48);
        ROUND4(D, A, B, C, 7, S42, 49);
        ROUND4(C, D, A, B, 14, S43, 50);
        ROUND4(B, C, D, A, 5, S44, 51);
        ROUND4(A, B, C, D, 12, S41, 52);
        ROUND4(D, A, B, C, 3, S42, 53);
        ROUND4(C, D, A, B, 10, S43, 54);
        ROUND4(B, C, D, A, 1, S44, 55);
        ROUND4(A, B, C, D, 8, S41, 56);
        ROUND4(D, A, B, C, 15, S42, 57);
        ROUND4(C, D, A, B, 6, S43, 58);
        ROUND4(B, C, D, A, 13, S44, 59);
        ROUND4(A, B, C, D, 4, S41, 60);
        ROUND4(D, A, B, C, 11, S42, 61);
        ROUND4(C, D, A, B, 2, S43, 62);
        ROUND4(B, C, D, A, 9, S44, 63);
#else
        ROUND1(A, B, C, D, 0, S11, 0xd76aa478);
        ROUND1(D, A, B, C, 1, S12, 0xe8c7b756);
        ROUND1(C, D, A, B, 2, S13, 0x242070db);
        ROUND1(B, C, D, A, 3, S14, 0xc1bdceee);
        ROUND1(A, B, C, D, 4, S11, 0xf57c0faf);
        ROUND1(D, A, B, C, 5, S12, 0x4787c62a);
        ROUND1(C, D, A, B, 6, S13, 0xa8304613);
        ROUND1(B, C, D, A, 7, S14, 0xfd469501);
        ROUND1(A, B, C, D, 8, S11, 0x698098d8);
        ROUND1(D, A, B, C, 9, S12, 0x8b44f7af);
        ROUND1(C, D, A, B, 10, S13, 0xffff5bb1);
        ROUND1(B, C, D, A, 11, S14, 0x895cd7be);
        ROUND1(A, B, C, D, 12, S11, 0x6b901122);
        ROUND1(D, A, B, C, 13, S12, 0xfd987193);
        ROUND1(C, D, A, B, 14, S13, 0xa679438e);
        ROUND1(B, C, D, A, 15, S14, 0x49b40821);

        ROUND2(A, B, C, D, 1, S21, 0xf61e2562);
        ROUND2(D, A, B, C, 6, S22, 0xc040b340);
        ROUND2(C, D, A, B, 11, S23, 0x265e5a51);
        ROUND2(B, C, D, A, 0, S24, 0xe9b6c7aa);
        ROUND2(A, B, C, D, 5, S21, 0xd62f105d);
        ROUND2(D, A, B, C, 10, S22, 0x2441453);
        ROUND2(C, D, A, B, 15, S23, 0xd8a1e681);
        ROUND2(B, C, D, A, 4, S24, 0xe7d3fbc8);
        ROUND2(A, B, C, D, 9, S21, 0x21e1cde6);
        ROUND2(D, A, B, C, 14, S22, 0xc33707d6);
        ROUND2(C, D, A, B, 3, S23, 0xf4d50d87);
        ROUND2(B, C, D, A, 8, S24, 0x455a14ed);
        ROUND2(A, B, C, D, 13, S21, 0xa9e3e905);
        ROUND2(D, A, B, C, 2, S22, 0xfcefa3f8);
        ROUND2(C, D, A, B, 7, S23, 0x676f02d9);
        ROUND2(B, C, D, A, 12, S24, 0x8d2a4c8a);

        ROUND3(A, B, C, D, 5, S31, 0xfffa3942);
        ROUND3(D, A, B, C, 8, S32, 0x8771f681);
        ROUND3(C, D, A, B, 11, S33, 0x6d9d6122);
        ROUND3(B, C, D, A, 14, S34, 0xfde5380c);
        ROUND3(A, B, C, D, 1, S31, 0xa4beea44);
        ROUND3(D, A, B, C, 4, S32, 0x4bdecfa9);
        ROUND3(C, D, A, B, 7, S33, 0xf6bb4b60);
        ROUND3(B, C, D, A, 10, S34, 0xbebfbc70);
        ROUND3(A, B, C, D, 13, S31, 0x289b7ec6);
        ROUND3(D, A, B, C, 0, S32, 0xeaa127fa);
        ROUND3(C, D, A, B, 3, S33, 0xd4ef3085);
        ROUND3(B, C, D, A, 6, S34, 0x4881d05);
        ROUND3(A, B, C, D, 9, S31, 0xd9d4d039);
        ROUND3(D, A, B, C, 12, S32, 0xe6db99e5);
        ROUND3(C, D, A, B, 15, S33, 0x1fa27cf8);
        ROUND3(B, C, D, A, 2, S34, 0xc4ac5665);

        ROUND4(A, B, C, D, 0, S41, 0xf4292244);
        ROUND4(D, A, B, C, 7, S42, 0x432aff97);
        ROUND4(C, D, A, B, 14, S43, 0xab9423a7);
        ROUND4(B, C, D, A, 5, S44, 0xfc93a039);
        ROUND4(A, B, C, D, 12, S41, 0x655b59c3);
        ROUND4(D, A, B, C, 3, S42, 0x8f0ccc92);
        ROUND4(C, D, A, B, 10, S43, 0xffeff47d);
        ROUND4(B, C, D, A, 1, S44, 0x85845dd1);
        ROUND4(A, B, C, D, 8, S41, 0x6fa87e4f);
        ROUND4(D, A, B, C, 15, S42, 0xfe2ce6e0);
        ROUND4(C, D, A, B, 6, S43, 0xa3014314);
        ROUND4(B, C, D, A, 13, S44, 0x4e0811a1);
        ROUND4(A, B, C, D, 4, S41, 0xf7537e82);
        ROUND4(D, A, B, C, 11, S42, 0xbd3af235);
        ROUND4(C, D, A, B, 2, S43, 0x2ad7d2bb);
        ROUND4(B, C, D, A, 9, S44, 0xeb86d391);
#endif
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
