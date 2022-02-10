add_library(process process_linux.cpp process_win.cpp)
target_link_libraries(process PRIVATE spdlog::spdlog)
