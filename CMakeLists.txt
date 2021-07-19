cmake_minimum_required(VERSION 3.15)

project(pdf-editor)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
enable_testing()

# Third Party Dependencies
set(BENCHMARK_ENABLE_TESTING OFF)
add_subdirectory(vendor/benchmark)
add_subdirectory(vendor/googletest)
add_subdirectory(vendor/zlib)

# Source Code
add_subdirectory(src)
