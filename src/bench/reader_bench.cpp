#include <benchmark/benchmark.h>

#include <pdf/document.h>

static void BM_Blank(benchmark::State &state) {
    for (auto _ : state) {
        pdf::Document document;
        pdf::Document::read_from_file("../../../test-files/blank.pdf", document);
    }
}
BENCHMARK(BM_Blank);

static void BM_HelloWorld(benchmark::State &state) {
    for (auto _ : state) {
        pdf::Document document;
        pdf::Document::read_from_file("../../../test-files/hello-world.pdf", document);
    }
}
BENCHMARK(BM_HelloWorld);

BENCHMARK_MAIN();
