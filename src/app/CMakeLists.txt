
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
    set(c_file ${CMAKE_CURRENT_BINARY_DIR}/${file_name}.c)
    add_custom_command(
            OUTPUT ${c_file} ${CMAKE_CURRENT_BINARY_DIR}/${file_name}.h
            COMMAND python3 ${CMAKE_SOURCE_DIR}/scripts/embed.py ${CMAKE_CURRENT_SOURCE_DIR}/${file} ${CMAKE_CURRENT_BINARY_DIR}/${file_name}
            DEPENDS ${file})
    set(${generated_c} ${c_file} PARENT_SCOPE)
endfunction()

add_subdirectory(cli)
add_subdirectory(common)
add_subdirectory(debugger)
add_subdirectory(editor)

install(TARGETS pdf-cli pdf-editor pdf-debugger DESTINATION bin)
