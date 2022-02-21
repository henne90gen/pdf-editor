
add_executable(bench_reader reader_bench.cpp)
target_link_libraries(bench_reader benchmark::benchmark pdf)
