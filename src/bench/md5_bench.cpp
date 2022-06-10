#include <benchmark/benchmark.h>

#include <hash/md5.h>

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

static void BM_Reference_abc(benchmark::State &state) {
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
BENCHMARK(BM_Reference_abc);

static void BM_abc(benchmark::State &state) {
    std::string testString = "abc";
    for (auto _ : state) {
        auto hash = hash::md5_checksum(testString);
        benchmark::DoNotOptimize(hash);
    }
}
BENCHMARK(BM_abc);

BENCHMARK_MAIN();
