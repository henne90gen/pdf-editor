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
else ()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif ()

# Source Code
enable_testing()
add_subdirectory(src)

# TODO Debian package: https://decovar.dev/blog/2021/09/23/cmake-cpack-package-deb-apt/#an-example-project
# TODO Window installer: https://dominoc925.blogspot.com/2016/09/create-windows-installer-using-cmake.html
# TODO improve installer texts

set(CPACK_PACKAGE_NAME "PDF-Editor")
set(CPACK_PACKAGE_VENDOR "Hendrik Müller")
set(CPACK_PACKAGE_CONTACT "henne90gen@gmail.com")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Hendrik Müller")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Open Source PDF Editor")
set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "0")
set(CPACK_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "PDF-Editor")

# TODO include LICENSE, as soon as it is available
#set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

# Define components and their display names
set(CPACK_COMPONENTS_ALL applications libraries headers)
set(CPACK_COMPONENT_APPLICATIONS_DISPLAY_NAME "Editor and Debugger")
set(CPACK_COMPONENT_LIBRARIES_DISPLAY_NAME "Libraries")
set(CPACK_COMPONENT_HEADERS_DISPLAY_NAME "C++ Headers")

# Human readable component descriptions
set(CPACK_COMPONENT_APPLICATIONS_DESCRIPTION "Open Source PDF Editor")
set(CPACK_COMPONENT_LIBRARIES_DESCRIPTION "Static libraries used to build programs with pdf")
set(CPACK_COMPONENT_HEADERS_DESCRIPTION "C/C++ header files for use with pdf")

# Define dependencies between components
set(CPACK_COMPONENT_HEADERS_DEPENDS libraries)

# Define groups
set(CPACK_COMPONENT_APPLICATIONS_GROUP "Runtime")
set(CPACK_COMPONENT_LIBRARIES_GROUP "Development")
set(CPACK_COMPONENT_HEADERS_GROUP "Development")

set(CPACK_COMPONENT_GROUP_DEVELOPMENT_DESCRIPTION "All of the tools you'll ever need to develop software")

# Define NSIS installation types
set(CPACK_ALL_INSTALL_TYPES Full User)
set(CPACK_COMPONENT_LIBRARIES_INSTALL_TYPES Full)
set(CPACK_COMPONENT_HEADERS_INSTALL_TYPES Full)
set(CPACK_COMPONENT_APPLICATIONS_INSTALL_TYPES Full User)

# https://unix.stackexchange.com/a/11552/254512
set(CPACK_PACKAGING_INSTALL_PREFIX "/opt/pdf-editor")
set(CPACK_VERBATIM_VARIABLES YES)

set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_COMPONENTS_GROUPING ALL_COMPONENTS_IN_ONE)
set(CPACK_DEB_COMPONENT_INSTALL YES)

include(CPack)
