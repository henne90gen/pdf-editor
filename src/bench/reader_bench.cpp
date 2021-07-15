#include <benchmark/benchmark.h>

#include <pdf_reader.h>

static void BM_Blank(benchmark::State& state) {
    for (auto _ : state) {
        pdf::File file;
        pdf::load_from_file("../../../test-files/blank.pdf", file);
    }
}
BENCHMARK(BM_Blank);

static void BM_HelloWorld(benchmark::State& state) {
    for (auto _ : state) {
        pdf::File file;
        pdf::load_from_file("../../../test-files/hello-world.pdf", file);
    }
}
BENCHMARK(BM_HelloWorld);

BENCHMARK_MAIN();
