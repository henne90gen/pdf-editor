
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtkmm-4.0)

function(target_link_gtk3 target)
    target_link_libraries(${target} PRIVATE ${GTK3_LIBRARIES})
    target_link_options(${target} PRIVATE ${GTK3_LDFLAGS_OTHER})
    target_compile_options(${target} PRIVATE ${GTK3_CFLAGS_OTHER})
    target_link_directories(${target} PRIVATE ${GTK3_LIBRARY_DIRS})
    target_include_directories(${target} PRIVATE ${GTK3_INCLUDE_DIRS})
endfunction()

function(embed_file file generated_c)
    get_filename_component(file_name ${file} NAME)
    add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${file_name}.c ${CMAKE_CURRENT_BINARY_DIR}/${file_name}.h
            COMMAND python3 ${CMAKE_SOURCE_DIR}/scripts/embed.py ${CMAKE_CURRENT_SOURCE_DIR}/${file} ${CMAKE_CURRENT_BINARY_DIR}/${file_name}
            DEPENDS ${file})
    set(${generated_c} ${CMAKE_CURRENT_BINARY_DIR}/${file_name}.c PARENT_SCOPE)
endfunction()

add_executable(pdf-cli cli/main.cpp util.cpp)
target_link_libraries(pdf-cli PRIVATE pdf-lib spdlog::spdlog)

embed_file(editor/editor.glade editor_glade)
add_executable(pdf-editor
        editor/main.cpp
        editor/EditorApplication.cpp
        editor/EditorWindow.cpp
        editor/PdfPage.cpp
        util.cpp
        ${editor_glade})
target_link_gtk3(pdf-editor)
target_link_libraries(pdf-editor PRIVATE pdf-lib spdlog::spdlog)
target_include_directories(pdf-editor PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

embed_file(debugger/debugger.glade debugger_glade)
add_executable(pdf-debugger
        debugger/main.cpp
        debugger/DebugApplication.cpp
        debugger/DebugWindow.cpp
        debugger/ContentContainer.cpp
        debugger/ContentArea.cpp
        debugger/ByteHighlightOverlay.cpp
        ${debugger_glade})
target_link_gtk3(pdf-debugger)
target_link_libraries(pdf-debugger PRIVATE pdf-lib spdlog::spdlog)
target_include_directories(pdf-debugger PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

install(TARGETS pdf-cli pdf-editor pdf-debugger DESTINATION bin)
