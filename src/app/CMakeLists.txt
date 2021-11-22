
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED gtkmm-4.0)

function(target_link_gtk4 target)
    target_link_libraries(${target} PRIVATE ${GTK4_LIBRARIES})
    target_link_options(${target} PRIVATE ${GTK4_LDFLAGS_OTHER})
    target_compile_options(${target} PRIVATE ${GTK4_CFLAGS_OTHER})
    target_link_directories(${target} PRIVATE ${GTK4_LIBRARY_DIRS})
    target_include_directories(${target} PRIVATE ${GTK4_INCLUDE_DIRS})
endfunction()

function(embed_file file generated_c)
    get_filename_component(file_name ${file} NAME)
    add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${file_name}.c ${CMAKE_CURRENT_BINARY_DIR}/${file_name}.h
            COMMAND python3 ${CMAKE_SOURCE_DIR}/scripts/embed.py ${CMAKE_CURRENT_SOURCE_DIR}/${file} ${CMAKE_CURRENT_BINARY_DIR}/${file_name}
            DEPENDS ${file})
    set(${generated_c} ${CMAKE_CURRENT_BINARY_DIR}/${file_name}.c PARENT_SCOPE)
endfunction()

add_executable(pdf-cli cli/main.cpp cli/util.cpp)
target_link_libraries(pdf-cli PRIVATE pdf-lib)

embed_file(common/open.xml open_xml)
embed_file(editor/editor.xml editor_xml)
embed_file(debugger/debugger.xml debugger_xml)

add_executable(pdf-editor
        editor/main.cpp
        editor/EditorWindow.cpp
        editor/PdfArea.cpp
        editor/PdfWindow.cpp
        editor/EditorApplication.cpp
        common/PdfApplication.cpp
        common/OpenWindow.cpp
        ${editor_xml}
        ${open_xml})
target_link_gtk4(pdf-editor)
target_link_libraries(pdf-editor PRIVATE pdf-lib)
target_include_directories(pdf-editor PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(pdf-debugger
        debugger/main.cpp
        debugger/DebugApplication.cpp
        debugger/DebugWindow.cpp
        debugger/ContentWindow.cpp
        debugger/ContentArea.cpp
        debugger/DocumentTree.cpp
        debugger/JumpToByteDialog.cpp
        common/PdfApplication.cpp
        common/OpenWindow.cpp
        ${debugger_xml}
        ${open_xml})
target_link_gtk4(pdf-debugger)
target_link_libraries(pdf-debugger PRIVATE pdf-lib)
target_include_directories(pdf-debugger PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

install(TARGETS pdf-cli pdf-editor pdf-debugger DESTINATION bin)
