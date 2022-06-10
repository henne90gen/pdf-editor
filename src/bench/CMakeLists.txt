
add_executable(reader_bench reader_bench.cpp)
target_link_libraries(reader_bench benchmark::benchmark pdf)

add_executable(writer_bench writer_bench.cpp)
target_link_libraries(writer_bench benchmark::benchmark pdf)

add_executable(md5_bench md5_bench.cpp md5_reference.c)
target_link_libraries(md5_bench benchmark::benchmark md5)
