include(GoogleTest)

add_executable(read_test lexer_test.cpp parser_test.cpp)
target_link_libraries(read_test gtest_main pdf-lib)
gtest_discover_tests(read_test)
