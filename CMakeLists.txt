cmake_minimum_required(VERSION 3.15)

project(pdf-editor)

# Compiler Settings
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
enable_testing()

# Third Party Dependencies
set(BENCHMARK_ENABLE_TESTING OFF)
add_subdirectory(vendor/benchmark EXCLUDE_FROM_ALL)
add_subdirectory(vendor/googletest EXCLUDE_FROM_ALL)
add_subdirectory(vendor/spdlog EXCLUDE_FROM_ALL)
add_subdirectory(vendor/zlib EXCLUDE_FROM_ALL)

# Source Code
add_subdirectory(src)
