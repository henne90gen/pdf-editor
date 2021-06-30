include(GoogleTest)

add_executable(ingest_test lexer_test.cpp parser_test.cpp)
target_link_libraries(ingest_test gtest_main pdf)
gtest_discover_tests(ingest_test)
