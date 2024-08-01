#include <benchmark/benchmark.h>

#include <pdf/memory/arena_allocator.h>

static void BM_ArenaCreate(benchmark::State &state) {
    for (auto _ : state) {
        size_t page_size_in_bytes = 1024 * 1024 * state.range(0);
        size_t maximumSizeInBytes = 1024L * 1024L * 1024L * state.range(0);
        auto result               = pdf::Arena::create(maximumSizeInBytes, page_size_in_bytes);
        ASSERT(!result.has_error());
        auto &arena = result.value();
        benchmark::DoNotOptimize(arena);
    }
}
BENCHMARK(BM_ArenaCreate)->Range(2, 1024);

static void BM_ArenaAllocate(benchmark::State &state) {
    size_t MB                 = 1024 * 1024;                       // 1 MB
    size_t page_size_in_bytes = 5 * MB;                            // 5 MB
    size_t maximumSizeInBytes = 1024LL * 1024LL * 1024LL * 1024LL; // 1 TB
    auto result               = pdf::Arena::create(maximumSizeInBytes, page_size_in_bytes);
    ASSERT(!result.has_error());
    auto &arena = result.value();
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
