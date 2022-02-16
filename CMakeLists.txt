cmake_minimum_required(VERSION 3.15)

project(pdf-editor VERSION 0.0.1)

# Third Party Dependencies
set(BENCHMARK_ENABLE_TESTING OFF)
set(BENCHMARK_ENABLE_INSTALL OFF)
set(BENCHMARK_INSTALL_DOCS OFF)
set(BUILD_GMOCK OFF)
set(INSTALL_GTEST OFF)
set(SKIP_INSTALL_ALL ON)

include(FetchContent)
FetchContent_Declare(
        benchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG 0d98dba29d66e93259db7daa53a9327df767a415 # v1.6.1
)
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG e2239ee6043f73722e7aa812a459f54a28552929 # release-1.11.0
)
FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG eb3220622e73a4889eee355ffa37972b3cac3df5 # v1.9.2
)
FetchContent_Declare(
        zlib
        GIT_REPOSITORY https://github.com/madler/zlib.git
        GIT_TAG cacf7f1d4e3d44d871b605da3b647f07d718623f # v1.2.11
)

FetchContent_MakeAvailable(
        benchmark
        googletest
        spdlog
        zlib
)

# Compiler Settings
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if (MSVC)
    add_compile_options(/W4 /WX)
elseif (NOT WIN32)
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif ()

# Source Code
enable_testing()
add_subdirectory(src)
