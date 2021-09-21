
find_package(PkgConfig REQUIRED)
pkg_check_modules(Cairo REQUIRED cairomm-1.0)

function(target_link_cairo target)
    target_link_libraries(${target} PUBLIC ${Cairo_LIBRARIES})
    target_link_options(${target} PUBLIC ${Cairo_LDFLAGS_OTHER})
    target_compile_options(${target} PUBLIC ${Cairo_CFLAGS_OTHER})
    message(STATUS "${Cairo_LIBRARY_DIRS}")
    message(STATUS "${Cairo_INCLUDE_DIRS}")
    target_link_directories(${target} PUBLIC ${Cairo_LIBRARY_DIRS})
    target_include_directories(${target} PUBLIC ${Cairo_INCLUDE_DIRS})
endfunction()

add_library(pdf-lib
        pdf/lexer.cpp
        pdf/parser.cpp
        pdf/document.cpp
        pdf/document_io.cpp
        pdf/font.cpp
        pdf/page.cpp
        pdf/objects.cpp
        pdf/cmap.cpp
        pdf/graphics.cpp
        pdf/text.cpp
        pdf/operator_parser.cpp
        pdf/renderer.cpp)
target_include_directories(pdf-lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(pdf-lib PUBLIC zlibstatic freetype spdlog::spdlog)
target_link_cairo(pdf-lib)
