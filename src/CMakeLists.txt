
add_subdirectory(app)
add_subdirectory(lib)

add_executable(pdf-test pdf_test.cpp)
target_link_libraries(pdf-test pdf-lib)
