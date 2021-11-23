#include <benchmark/benchmark.h>

#include <pdf/helper/allocator.h>
#include <pdf/helper/static_vector.h>

static void SV_Create(benchmark::State &state) {
    pdf::Allocator allocator = {};
    allocator.init(10000);
    std::vector<int> v = {1, 2, 3, 4};
    for (auto _ : state) {
        allocator.clear_current_allocation();

        auto sv = pdf::StaticVector<int>::create(allocator, v);
        benchmark::DoNotOptimize(sv);
    }
}
BENCHMARK(SV_Create);

static void SV_Remove(benchmark::State &state) {
    pdf::Allocator allocator = {};
    allocator.init(10000);
    std::vector<int> v = {1, 2, 3, 4};
    for (auto _ : state) {
        allocator.clear_current_allocation();
        auto sv = pdf::StaticVector<int>::create(allocator, v);

        sv.remove(0);
        sv.remove(0);
        sv.remove(0);
        sv.remove(0);
    }
}
BENCHMARK(SV_Remove);

static void SV_Iterate(benchmark::State &state) {
    pdf::Allocator allocator = {};
    allocator.init(10000);
    std::vector<int> v = {1, 2, 3, 4};
    for (auto _ : state) {
        allocator.clear_current_allocation();

        auto sv = pdf::StaticVector<int>::create(allocator, v);
        for (auto &elem : sv) {
            benchmark::DoNotOptimize(elem);
        }
    }
}
BENCHMARK(SV_Iterate);

static void V_Create(benchmark::State &state) {
    pdf::Allocator allocator = {};
    allocator.init(10000);
    for (auto _ : state) {
        allocator.clear_current_allocation();

        std::vector<int> v = {1, 2, 3, 4};
        benchmark::DoNotOptimize(v);
    }
}
BENCHMARK(V_Create);

static void V_Remove(benchmark::State &state) {
    pdf::Allocator allocator = {};
    allocator.init(10000);
    for (auto _ : state) {
        allocator.clear_current_allocation();
        std::vector<int> v = {1, 2, 3, 4};

        v.erase(v.begin());
        v.erase(v.begin());
        v.erase(v.begin());
        v.erase(v.begin());
    }
}
BENCHMARK(V_Remove);

static void V_Iterate(benchmark::State &state) {
    pdf::Allocator allocator = {};
    allocator.init(10000);
    for (auto _ : state) {
        allocator.clear_current_allocation();

        std::vector<int> v = {1, 2, 3, 4};
        for (auto &elem : v) {
            benchmark::DoNotOptimize(elem);
        }
    }
}
BENCHMARK(V_Iterate);

BENCHMARK_MAIN();
