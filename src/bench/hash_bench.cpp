#include <benchmark/benchmark.h>

#include <fstream>
#include <hash/md5.h>
#include <hash/sha1.h>
#include <iostream>

typedef unsigned long int UINT4;
struct MD5_CTX {
    UINT4 state[4];           /* state (ABCD) */
    UINT4 count[2];           /* number of bits, modulo 2^64 (lsb first) */
    unsigned char buffer[64]; /* input buffer */
};
extern "C" void MD5Init(MD5_CTX *);
extern "C" void MD5Update(MD5_CTX *, unsigned char *, unsigned int);
extern "C" void MD5Final(unsigned char[16], MD5_CTX *);
extern "C" void MDPrint(unsigned char digest[16]);

typedef struct SHA1Context {
    uint32_t Intermediate_Hash[5]; /* Message Digest  */

    uint32_t Length_Low;  /* Message length in bits      */
    uint32_t Length_High; /* Message length in bits      */

    /* Index into message block array   */
    int_least16_t Message_Block_Index;
    uint8_t Message_Block[64]; /* 512-bit message blocks      */

    int Computed;  /* Is the digest computed?         */
    int Corrupted; /* Is the message digest corrupted? */
} SHA1Context;
extern "C" int SHA1Reset(SHA1Context *);
extern "C" int SHA1Input(SHA1Context *, const uint8_t *, unsigned int);
extern "C" int SHA1Result(SHA1Context *, uint8_t Message_Digest[20]);

std::pair<uint8_t *, uint64_t> read_file(const std::string &fileName) {
    std::ifstream file(fileName, std::ios::binary | std::ios::ate);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    auto buffer = (uint8_t *)malloc(size);
    if (!file.read(reinterpret_cast<char *>(buffer), size)) {
        std::cerr << "Failed to read file" << std::endl;
        exit(1);
    }
    return {buffer, size};
}

static void BM_MD5_Reference_abc(benchmark::State &state) {
    std::string testString = "abc";
    for (auto _ : state) {
        MD5_CTX ctx = {};
        MD5Init(&ctx);
        MD5Update(&ctx, (uint8_t *)testString.c_str(), testString.size());
        unsigned char digest[16];
        MD5Final(digest, &ctx);
        benchmark::DoNotOptimize(digest);
    }
}
BENCHMARK(BM_MD5_Reference_abc);

static void BM_MD5_abc(benchmark::State &state) {
    std::string testString = "abc";
    for (auto _ : state) {
        auto hash = hash::md5_checksum(testString);
        benchmark::DoNotOptimize(hash);
    }
}
BENCHMARK(BM_MD5_abc);

static void BM_MD5_Reference_File(benchmark::State &state) {
    auto file = read_file("../../../test-files/image-1.pdf");
    for (auto _ : state) {
        MD5_CTX ctx = {};
        MD5Init(&ctx);
        MD5Update(&ctx, file.first, file.second);
        unsigned char digest[16];
        MD5Final(digest, &ctx);
        benchmark::DoNotOptimize(digest);
    }
}
BENCHMARK(BM_MD5_Reference_File);

static void BM_MD5_File(benchmark::State &state) {
    auto file = read_file("../../../test-files/image-1.pdf");
    for (auto _ : state) {
        auto hash = hash::md5_checksum(file.first, file.second);
        benchmark::DoNotOptimize(hash);
    }
}
BENCHMARK(BM_MD5_File);

static void BM_SHA1_Reference_abc(benchmark::State &state) {
    std::string testString = "abc";
    for (auto _ : state) {
        SHA1Context ctx = {};
        SHA1Reset(&ctx);
        SHA1Input(&ctx, (uint8_t *)testString.c_str(), testString.size());
        unsigned char digest[16];
        SHA1Result(&ctx, digest);
        benchmark::DoNotOptimize(digest);
    }
}
BENCHMARK(BM_SHA1_Reference_abc);

static void BM_SHA1_abc(benchmark::State &state) {
    std::string testString = "abc";
    for (auto _ : state) {
        auto hash = hash::sha1_checksum(testString);
        benchmark::DoNotOptimize(hash);
    }
}
BENCHMARK(BM_SHA1_abc);

static void BM_SHA1_Reference_File(benchmark::State &state) {
    auto file = read_file("../../../test-files/image-1.pdf");
    for (auto _ : state) {
        SHA1Context ctx = {};
        SHA1Reset(&ctx);
        SHA1Input(&ctx, file.first, file.second);
        unsigned char digest[16];
        SHA1Result(&ctx, digest);
        benchmark::DoNotOptimize(digest);
    }
}
BENCHMARK(BM_SHA1_Reference_File);

static void BM_SHA1_File(benchmark::State &state) {
    auto file = read_file("../../../test-files/image-1.pdf");
    for (auto _ : state) {
        auto hash = hash::sha1_checksum(file.first, file.second);
        benchmark::DoNotOptimize(hash);
    }
}
BENCHMARK(BM_SHA1_File);

BENCHMARK_MAIN();
