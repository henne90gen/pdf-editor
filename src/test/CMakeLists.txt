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

add_executable(write_test writer_test.cpp)
target_link_libraries(write_test gtest_main pdf-lib)
gtest_discover_tests(write_test)

add_custom_target(test_files
        COMMAND python3 ${CMAKE_SOURCE_DIR}/scripts/test_directory.py $<TARGET_FILE:pdf-cli> ${CMAKE_SOURCE_DIR}/test-files)
add_dependencies(test_files pdf-cli)

add_custom_target(download_test_suites
        COMMAND python3 ${CMAKE_SOURCE_DIR}/scripts/download_test_suites.py ${CMAKE_CURRENT_BINARY_DIR}/test-suites)

add_custom_target(test_suite_verapdf
        COMMAND python3 ${CMAKE_SOURCE_DIR}/scripts/integration_test.py $<TARGET_FILE:pdf-cli> ${CMAKE_CURRENT_BINARY_DIR}/test-suites VeraPDF)
add_dependencies(test_suite_verapdf pdf-cli download_test_suites)

add_custom_target(test_suite_isartor
        COMMAND python3 ${CMAKE_SOURCE_DIR}/scripts/integration_test.py $<TARGET_FILE:pdf-cli> ${CMAKE_CURRENT_BINARY_DIR}/test-suites Isartor)
add_dependencies(test_suite_isartor pdf-cli download_test_suites)

add_custom_target(test_suite_bfosupport
        COMMAND python3 ${CMAKE_SOURCE_DIR}/scripts/integration_test.py $<TARGET_FILE:pdf-cli> ${CMAKE_CURRENT_BINARY_DIR}/test-suites BFOSupport)
add_dependencies(test_suite_bfosupport pdf-cli download_test_suites)
