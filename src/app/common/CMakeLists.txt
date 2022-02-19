#add_custom_command(
#        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/resources.c
#        COMMAND glib-compile-resources common.gresource.xml --target=${CMAKE_CURRENT_BINARY_DIR}/resources.c --generate-source
#        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
#        DEPENDS common.gresource.xml open.ui)
embed_file(open.ui open_ui)
add_library(pdf-common STATIC
        PdfApplication.cpp
        OpenWindow.cpp
        ScrolledZoomedWindow.cpp
        ${open_ui})
target_link_gtk4(pdf-common)
target_link_libraries(pdf-common PRIVATE spdlog::spdlog)
target_include_directories(pdf-common PUBLIC ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
