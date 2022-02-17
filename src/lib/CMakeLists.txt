
find_package(PkgConfig REQUIRED)
pkg_check_modules(Cairo REQUIRED cairomm-1.16)

function(target_link_cairo target)
    target_link_libraries(${target} PUBLIC ${Cairo_LIBRARIES})
    target_link_options(${target} PUBLIC ${Cairo_LDFLAGS_OTHER})
    target_compile_options(${target} PUBLIC ${Cairo_CFLAGS_OTHER})
    target_link_directories(${target} PUBLIC ${Cairo_LIBRARY_DIRS})
    target_include_directories(${target} PUBLIC ${Cairo_INCLUDE_DIRS})
endfunction()

add_library(pdf
        pdf/lexer.cpp
        pdf/parser.cpp
        pdf/document.cpp
        pdf/document_read.cpp
        pdf/document_write.cpp
        pdf/font.cpp
        pdf/page.cpp
        pdf/objects.cpp
        pdf/cmap.cpp
        pdf/operator_parser.cpp
        pdf/image.cpp
        pdf/renderer.cpp
        pdf/operator_traverser.cpp)
target_include_directories(pdf PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(pdf PUBLIC zlibstatic freetype spdlog::spdlog util)
target_link_cairo(pdf)

install(TARGETS pdf zlibstatic spdlog
        EXPORT pdf
        ARCHIVE
        COMPONENT libraries)
install(DIRECTORY pdf TYPE INCLUDE COMPONENT headers FILES_MATCHING PATTERN "*.h")
install(FILES pdf-config.cmake TYPE LIB COMPONENT libraries)
install(EXPORT pdf DESTINATION lib)
