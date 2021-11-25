
add_executable(pdf-cli
        main.cpp
        util.cpp)
target_link_libraries(pdf-cli PRIVATE pdf-lib)
