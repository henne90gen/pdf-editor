
add_executable(reader_bench reader_bench.cpp)
target_link_libraries(reader_bench benchmark::benchmark pdf-lib)
