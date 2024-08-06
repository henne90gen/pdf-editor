#include <benchmark/benchmark.h>

#include <pdf/document.h>

static void BM_Blank(benchmark::State &state) {
    auto allocatorResult = pdf::Allocator::create();
    assert(not allocatorResult.has_error());

    for (auto _ : state) {
        auto result = pdf::Document::read_from_file(allocatorResult.value(), "../../../test-files/blank.pdf");
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_Blank);

static void BM_HelloWorld(benchmark::State &state) {
    auto allocatorResult = pdf::Allocator::create();
    assert(not allocatorResult.has_error());

    for (auto _ : state) {
        auto result = pdf::Document::read_from_file(allocatorResult.value(), "../../../test-files/hello-world.pdf");
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_HelloWorld);

BENCHMARK_MAIN();
