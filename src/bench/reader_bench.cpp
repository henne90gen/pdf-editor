#include <benchmark/benchmark.h>

#include <pdf/document.h>

static void BM_Blank(benchmark::State &state) {
    for (auto _ : state) {
        auto result = pdf::Document::read_from_file("../../../test-files/blank.pdf");
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_Blank);

static void BM_HelloWorld(benchmark::State &state) {
    for (auto _ : state) {
        auto result = pdf::Document::read_from_file("../../../test-files/hello-world.pdf");
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_HelloWorld);

BENCHMARK_MAIN();
