add_library(pdf pdf_reader.cpp Lexer.cpp Parser.cpp)
target_include_directories(pdf PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(pdf-test pdf_test.cpp)
target_link_libraries(pdf-test pdf)


find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtkmm-3.0)

add_executable(pdf-editor main.cpp)
target_link_libraries(pdf-editor ${GTK3_LIBRARIES} pdf)
target_link_options(pdf-editor PRIVATE ${GTK3_LDFLAGS_OTHER})
target_compile_options(pdf-editor PRIVATE ${GTK3_CFLAGS_OTHER})
target_link_directories(pdf-editor PRIVATE ${GTK3_LIBRARY_DIRS})
target_include_directories(pdf-editor PRIVATE ${GTK3_INCLUDE_DIRS})
