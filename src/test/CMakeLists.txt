include(GoogleTest)

add_executable(read_test
        lexer_test.cpp
        parser_test.cpp
        reader_test.cpp
        operator_parser_test.cpp
        cmap_parser_test.cpp)
target_link_libraries(read_test gtest_main pdf-lib)
gtest_discover_tests(read_test)

add_executable(render_test render_test.cpp)
target_link_libraries(render_test gtest_main pdf-lib)
gtest_discover_tests(render_test)
