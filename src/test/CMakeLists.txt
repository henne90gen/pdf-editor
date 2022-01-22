include(GoogleTest)

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

function(create_test test_name)
    add_executable(${test_name} ${test_name}.cpp)
    target_link_libraries(${test_name} gtest_main pdf)
    gtest_discover_tests(${test_name})
endfunction()

create_test(cmap_parser_test)
create_test(lexer_test)
create_test(operator_parser_test)
create_test(parser_test)
create_test(reader_test)
create_test(render_test)
create_test(static_map_test)
create_test(static_vector_test)
create_test(text_test)
create_test(writer_test)
create_test(write_object_test)
