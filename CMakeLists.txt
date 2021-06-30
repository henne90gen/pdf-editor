cmake_minimum_required(VERSION 3.15)

project(pdf-editor)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
enable_testing()

add_subdirectory(vendor/googletest)
add_subdirectory(src)
add_subdirectory(test)
