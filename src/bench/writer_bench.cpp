#include <benchmark/benchmark.h>

#include <pdf/document.h>

static void BM_Blank(benchmark::State &state) {
    pdf::Document document;
    pdf::Document::read_from_file("../../../test-files/blank.pdf", document);
    for (auto _ : state) {
        char *buffer = nullptr;
        size_t size  = 0;
        auto result  = document.write_to_memory(buffer, size);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_Blank);

static void BM_HelloWorld(benchmark::State &state) {
    pdf::Document document;
    pdf::Document::read_from_file("../../../test-files/hello-world.pdf", document);
    for (auto _ : state) {
        char *buffer = nullptr;
        size_t size  = 0;
        auto result  = document.write_to_memory(buffer, size);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_HelloWorld);

BENCHMARK_MAIN();
