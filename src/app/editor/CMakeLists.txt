embed_file(editor.xml editor_xml)
add_executable(pdf-editor
        main.cpp
        EditorWindow.cpp
        PdfArea.cpp
        PdfWindow.cpp
        EditorApplication.cpp
        ${editor_xml})
target_link_gtk4(pdf-editor)
target_link_libraries(pdf-editor PRIVATE pdf-lib pdf-common)
target_include_directories(pdf-editor PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
