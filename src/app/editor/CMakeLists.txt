add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/resources.c
        COMMAND glib-compile-resources editor.gresource.xml --target=${CMAKE_CURRENT_BINARY_DIR}/resources.c --generate-source
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        DEPENDS editor.gresource.xml editor.ui)
add_executable(pdf-editor
        main.cpp
        EditorWindow.cpp
        PdfArea.cpp
        PdfWindow.cpp
        EditorApplication.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/resources.c)
target_link_gtk4(pdf-editor)
target_link_adwaita(pdf-editor)
target_link_libraries(pdf-editor PRIVATE pdf)
target_include_directories(pdf-editor PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
