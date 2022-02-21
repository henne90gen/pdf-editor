
add_executable(bench_reader reader_bench.cpp)
target_link_libraries(bench_reader benchmark::benchmark pdf)

add_executable(bench_writer writer_bench.cpp)
target_link_libraries(bench_writer benchmark::benchmark pdf)
