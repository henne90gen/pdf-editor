
add_executable(pdf-cli main.cpp)
target_link_libraries(pdf-cli PUBLIC pdf-lib spdlog::spdlog)
