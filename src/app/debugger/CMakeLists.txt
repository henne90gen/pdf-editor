embed_file(debugger.xml debugger_xml)
add_executable(pdf-debugger
        main.cpp
        DebugApplication.cpp
        DebugWindow.cpp
        ContentWindow.cpp
        ContentArea.cpp
        DocumentTree.cpp
        JumpToByteDialog.cpp
        ${debugger_xml})
target_link_gtk4(pdf-debugger)
target_link_libraries(pdf-debugger PRIVATE pdf pdf-common)
target_include_directories(pdf-debugger PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
