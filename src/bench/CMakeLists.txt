
add_executable(reader_bench reader_bench.cpp)
target_link_libraries(reader_bench benchmark::benchmark pdf)

add_executable(writer_bench writer_bench.cpp)
target_link_libraries(writer_bench benchmark::benchmark pdf)

add_executable(hash_bench hash_bench.cpp)
target_link_libraries(hash_bench
        benchmark::benchmark
        hash
        md5_reference
        sha1_reference)
