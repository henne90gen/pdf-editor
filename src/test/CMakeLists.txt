include(GoogleTest)

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
    add_executable(${test_name} ${test_name}.cpp process_win.cpp process_linux.cpp)
    target_link_libraries(${test_name} gtest_main pdf)
    gtest_discover_tests(${test_name})
endfunction()

create_test(allocator_test)
create_test(cmap_parser_test)
create_test(image_test)
create_test(lexer_test)
create_test(operator_parser_test)
create_test(parser_test)
create_test(reader_test)
create_test(render_test)
create_test(text_test)
create_test(writer_test)
create_test(write_object_test)

if (NOT WIN32)
    add_executable(fuzzer fuzzer.cpp)
    target_link_libraries(fuzzer pdf)
    target_link_options(fuzzer PRIVATE -fsanitize=fuzzer,address)
    target_compile_options(fuzzer PRIVATE -fsanitize=fuzzer,address)

    add_custom_target(test_fuzzer
            COMMAND $<TARGET_FILE:fuzzer> -runs=0 fuzzing-corpus
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
endif ()

add_subdirectory(hash)
