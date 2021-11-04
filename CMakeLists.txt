cmake_minimum_required(VERSION 3.15)

project(pdf-editor)

# Compiler Settings
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if (MSVC)
    add_compile_options(/W4 /WX)
elseif(NOT WIN32)
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif ()

# Third Party Dependencies
set(BENCHMARK_ENABLE_TESTING OFF)
add_subdirectory(vendor/benchmark EXCLUDE_FROM_ALL)

add_subdirectory(vendor/googletest EXCLUDE_FROM_ALL)
add_subdirectory(vendor/spdlog EXCLUDE_FROM_ALL)

set(SKIP_INSTALL_ALL ON)
add_subdirectory(vendor/zlib)

# Source Code
enable_testing()
add_subdirectory(src)
