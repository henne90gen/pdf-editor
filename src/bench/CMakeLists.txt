
add_executable(reader_bench reader_bench.cpp)
target_link_libraries(reader_bench benchmark::benchmark pdf)

add_executable(static_vector_bench static_vector_bench.cpp)
target_link_libraries(static_vector_bench benchmark::benchmark pdf)

add_executable(static_map_bench static_map_bench.cpp)
target_link_libraries(static_map_bench benchmark::benchmark pdf)
