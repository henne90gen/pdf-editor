
add_executable(bench_reader reader_bench.cpp)
target_link_libraries(bench_reader benchmark::benchmark pdf)

add_executable(bench_writer writer_bench.cpp)
target_link_libraries(bench_writer benchmark::benchmark pdf)

add_executable(bench_md5 md5_bench.cpp md5_reference.c)
target_link_libraries(bench_md5 benchmark::benchmark md5)
