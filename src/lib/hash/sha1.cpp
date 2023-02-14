#include "sha1.h"

#include "hex_string.h"

namespace hash {

#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

static inline uint32_t F(uint32_t t, uint32_t B, uint32_t C, uint32_t D) {
    if (t <= 19) {
        return (B & C) | (~B & D);
    } else if (t <= 39 || t > 59) {
        return B ^ C ^ D;
    } else if (t <= 59) {
        return (B & C) | (B & D) | (C & D);
    }
    return 0;
}

static inline uint32_t K(uint32_t t) {
    if (t <= 19) {
        return 0x5A827999;
    } else if (t <= 39) {
        return 0x6ED9EBA1;
    } else if (t <= 59) {
        return 0x8F1BBCDC;
    } else {
        return 0xCA62C1D6;
    }
}

SHA1Hash sha1_checksum(const std::string &str) { return sha1_checksum((uint8_t *)str.c_str(), str.size()); }

inline void fill_X_and_apply_padding(std::array<uint8_t, 64> &X, const uint8_t *bytesIn, uint64_t offset,
                                     uint64_t sizeInBytesIn, uint64_t sizeInBytesPadded) {
    std::memcpy(X.data(), bytesIn + offset, std::min((uint64_t)64, sizeInBytesIn - offset));
    if (offset + 64 <= sizeInBytesIn) {
        return;
    }

    if (offset < sizeInBytesIn) {
        X[sizeInBytesIn % 64] = 0b10000000;
        for (size_t j = (sizeInBytesIn % 64) + 1;
             j < std::max(sizeInBytesPadded % 64, sizeInBytesPadded - sizeInBytesIn); j++) {
            X[j] = 0;
        }
    } else {
        for (size_t j = 0; j < 56; j++) {
            X[j] = 0;
        }
        if (offset == sizeInBytesIn) {
            X[sizeInBytesIn % 64] = 0b10000000;
        }
    }

    if (sizeInBytesIn % 64 < 56 || offset > sizeInBytesIn) {
        uint64_t sizeInBits = sizeInBytesIn << 3;

        X[56] = sizeInBits >> 56;
        X[57] = sizeInBits >> 48;
        X[58] = sizeInBits >> 40;
        X[59] = sizeInBits >> 32;
        X[60] = sizeInBits >> 24;
        X[61] = sizeInBits >> 16;
        X[62] = sizeInBits >> 8;
        X[63] = sizeInBits;

        //        auto ptr = (uint64_t *)((X.data()) + (sizeInBytesPadded % 64));
        //        *ptr     = sizeInBits;
    }
}

SHA1Hash sha1_checksum(const uint8_t *bytes, uint64_t sizeInBytes) {
    uint64_t sizeInBytesPadded = calculate_padded_size(sizeInBytes);
    uint64_t sizeInBytesFinal  = sizeInBytesPadded + 8;

    std::array<uint32_t, 5> H = {0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0};

    // process input in 16-word (64 byte) chunks
    std::array<uint8_t, 64> X  = {};
    std::array<uint32_t, 80> W = {};
    for (uint64_t i = 0; i < sizeInBytesFinal / 64; i++) {
        auto offset = i * 64;
        fill_X_and_apply_padding(X, bytes, offset, sizeInBytes, sizeInBytesPadded);

        for (uint32_t t = 0; t < 16; t++) {
            W[t] = X[t * 4] << 24;
            W[t] |= X[t * 4 + 1] << 16;
            W[t] |= X[t * 4 + 2] << 8;
            W[t] |= X[t * 4 + 3];
        }

        for (uint32_t t = 16; t <= 79; t++) {
            W[t] = ROTATE_LEFT(W[t - 3] ^ W[t - 8] ^ W[t - 14] ^ W[t - 16], 1);
        }

        uint32_t A = H[0];
        uint32_t B = H[1];
        uint32_t C = H[2];
        uint32_t D = H[3];
        uint32_t E = H[4];

        for (uint32_t t = 0; t <= 79; t++) {
            auto TEMP = ROTATE_LEFT(A, 5) + F(t, B, C, D) + E + W[t] + K(t);

            E = D;
            D = C;
            C = ROTATE_LEFT(B, 30);
            B = A;
            A = TEMP;
        }

        H[0] += A;
        H[1] += B;
        H[2] += C;
        H[3] += D;
        H[4] += E;
    }

    return H;
}

} // namespace hash
