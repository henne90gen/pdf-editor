include(GoogleTest)

add_executable(hash_test md5_test.cpp sha1_test.cpp util_test.cpp)
target_link_libraries(hash_test gtest_main pdf)
gtest_discover_tests(hash_test)
