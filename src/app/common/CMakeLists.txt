embed_file(open.xml open_xml)
add_library(pdf-common STATIC
    ${open_xml}
    PdfApplication.cpp
    OpenWindow.cpp
    ScrollableContentWindow.cpp)
target_link_gtk4(pdf-common)
target_link_libraries(pdf-common PRIVATE spdlog::spdlog)
target_include_directories(pdf-common PUBLIC ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
