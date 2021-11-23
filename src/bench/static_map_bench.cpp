#include <benchmark/benchmark.h>

#include <pdf/helper/allocator.h>
#include <pdf/helper/static_map.h>

static void SM_Create(benchmark::State &state) {
    pdf::Allocator allocator = {};
    allocator.init(10000);
    std::unordered_map<int, int> m = {{1, 2}, {2, 3}, {3, 4}, {4, 5}};
    for (auto _ : state) {
        allocator.clear_current_allocation();

        auto sm = pdf::StaticMap<int, int>::create(allocator, m);
        benchmark::DoNotOptimize(sm);
    }
}
BENCHMARK(SM_Create);

static void SM_Remove(benchmark::State &state) {
    pdf::Allocator allocator = {};
    allocator.init(10000);
    std::unordered_map<int, int> m = {{1, 2}, {2, 3}, {3, 4}, {4, 5}};
    for (auto _ : state) {
        allocator.clear_current_allocation();

        auto sm = pdf::StaticMap<int, int>::create(allocator, m);
        sm.remove(1);
        sm.remove(2);
        sm.remove(3);
        sm.remove(4);
    }
}
BENCHMARK(SM_Remove);

static void SM_Iterate(benchmark::State &state) {
    pdf::Allocator allocator = {};
    allocator.init(10000);
    std::unordered_map<int, int> m = {{1, 2}, {2, 3}, {3, 4}, {4, 5}};
    for (auto _ : state) {
        allocator.clear_current_allocation();

        auto sm = pdf::StaticMap<int, int>::create(allocator, m);
        for (auto &elem : sm) {
            benchmark::DoNotOptimize(elem);
        }
    }
}
BENCHMARK(SM_Iterate);

static void M_Create(benchmark::State &state) {
    pdf::Allocator allocator = {};
    allocator.init(10000);
    for (auto _ : state) {
        allocator.clear_current_allocation();

        std::unordered_map<int, int> m = {{1, 2}, {2, 3}, {3, 4}, {4, 5}};
        benchmark::DoNotOptimize(m);
    }
}
BENCHMARK(M_Create);

static void M_Remove(benchmark::State &state) {
    pdf::Allocator allocator = {};
    allocator.init(10000);
    for (auto _ : state) {
        allocator.clear_current_allocation();

        std::unordered_map<int, int> m = {{1, 2}, {2, 3}, {3, 4}, {4, 5}};
        m.erase(1);
        m.erase(2);
        m.erase(3);
        m.erase(4);
    }
}
BENCHMARK(M_Remove);

static void M_Iterate(benchmark::State &state) {
    pdf::Allocator allocator = {};
    allocator.init(10000);
    for (auto _ : state) {
        allocator.clear_current_allocation();

        std::unordered_map<int, int> m = {{1, 2}, {2, 3}, {3, 4}, {4, 5}};
        for (auto &elem : m) {
            benchmark::DoNotOptimize(elem.first);
            benchmark::DoNotOptimize(elem.second);
        }
    }
}
BENCHMARK(M_Iterate);

BENCHMARK_MAIN();
