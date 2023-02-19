#include <benchmark/benchmark.h>

#include <pdf/memory/arena_allocator.h>

static void BM_ArenaCreate(benchmark::State &state) {
    for (auto _ : state) {
        size_t pageSize           = 1024 * 1024 * state.range(0);
        size_t maximumSizeInBytes = 1024L * 1024L * 1024L * state.range(0);
        const auto arena          = pdf::Arena(maximumSizeInBytes, pageSize);
        benchmark::DoNotOptimize(arena);
    }
}
BENCHMARK(BM_ArenaCreate)->Range(2, 1024);

static void BM_ArenaAllocate(benchmark::State &state) {
    size_t pageSize           = 1024 * 1024 * 1024;
    size_t maximumSizeInBytes = 1024L * 1024L * 1024L * 1024L;
    auto arena                = pdf::Arena(maximumSizeInBytes, pageSize);
    for (auto _ : state) {
        const auto buf = arena.push(state.range(0));
        benchmark::DoNotOptimize(buf);
    }
}
BENCHMARK(BM_ArenaAllocate)->Range(2, 1024);

static void BM_MallocAllocate(benchmark::State &state) {
    for (auto _ : state) {
        const auto buf = malloc(state.range(0));
        benchmark::DoNotOptimize(buf);
    }
}
BENCHMARK(BM_MallocAllocate)->Range(2, 1024);

BENCHMARK_MAIN();
