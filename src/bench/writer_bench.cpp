#include <benchmark/benchmark.h>

#include <pdf/document.h>

static void BM_Blank(benchmark::State &state) {
    auto allocatorResult = pdf::Allocator::create();
    assert(not allocatorResult.has_error());

    auto documentResult = pdf::Document::read_from_file(allocatorResult.value(), "../../../test-files/blank.pdf");
    auto &document      = documentResult.value();
    for (auto _ : state) {
        uint8_t *buffer = nullptr;
        size_t size     = 0;
        auto result     = document.write_to_memory(buffer, size);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_Blank);

static void BM_HelloWorld(benchmark::State &state) {
    auto allocatorResult = pdf::Allocator::create();
    assert(not allocatorResult.has_error());

    auto documentResult = pdf::Document::read_from_file(allocatorResult.value(), "../../../test-files/hello-world.pdf");
    auto &document      = documentResult.value();
    for (auto _ : state) {
        uint8_t *buffer = nullptr;
        size_t size     = 0;
        auto result     = document.write_to_memory(buffer, size);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_HelloWorld);

BENCHMARK_MAIN();
