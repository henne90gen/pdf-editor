add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/resources.c
        COMMAND glib-compile-resources debugger.gresource.xml --target=${CMAKE_CURRENT_BINARY_DIR}/resources.c --generate-source
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        DEPENDS debugger.gresource.xml debugger.ui style.css)
add_executable(pdf-debugger
        main.cpp
        DebugApplication.cpp
        DebugWindow.cpp
        ContentWindow.cpp
        ContentArea.cpp
        ObjectList.cpp
        JumpToByteDialog.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/resources.c)
target_link_gtk4(pdf-debugger)
target_link_libraries(pdf-debugger PRIVATE pdf pdf-common)
target_include_directories(pdf-debugger PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
