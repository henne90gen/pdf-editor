
add_executable(bench_reader reader_bench.cpp)
target_link_libraries(bench_reader benchmark::benchmark pdf)

add_executable(bench_static_vector static_vector_bench.cpp)
target_link_libraries(bench_static_vector benchmark::benchmark pdf)

add_executable(bench_static_map static_map_bench.cpp)
target_link_libraries(bench_static_map benchmark::benchmark pdf)
