add_library(pdf-common STATIC
        PdfApplication.cpp
        OpenWindow.cpp
        ScrolledZoomedWindow.cpp)
target_link_gtk4mm(pdf-common)
target_link_libraries(pdf-common PRIVATE spdlog::spdlog)
target_include_directories(pdf-common PUBLIC ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
