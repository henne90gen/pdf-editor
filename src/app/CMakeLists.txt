
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

if (MINGW)
    get_filename_component(MINGW_PATH ${CMAKE_CXX_COMPILER} PATH)

    # GLib Schemas
    add_custom_target(copy_glib_schemas
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${MINGW_PATH}/../share/glib-2.0/schemas
            ${CMAKE_CURRENT_BINARY_DIR}/share/glib-2.0/schemas)
    add_custom_target(compile_glib_schemas
            COMMAND glib-compile-schemas ${CMAKE_CURRENT_BINARY_DIR}/share/glib-2.0/schemas)
    add_dependencies(compile_glib_schemas copy_glib_schemas)
    add_dependencies(pdf-editor compile_glib_schemas)
    add_dependencies(pdf-debugger compile_glib_schemas)

    install(TARGETS pdf-cli pdf-editor pdf-debugger
            RUNTIME_DEPENDENCIES
            DIRECTORIES ${MINGW_PATH}
            PRE_EXCLUDE_REGEXES "api-ms-" "ext-ms-"
            POST_EXCLUDE_REGEXES ".*system32/.*\\.dll" >
            RUNTIME DESTINATION bin
            COMPONENT applications)
    install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/share/glib-2.0/schemas
            DESTINATION share/glib-2.0
            COMPONENT applications)
    install(DIRECTORY ${MINGW_PATH}/../share/icons
            DESTINATION share
            COMPONENT applications)
else ()
    install(TARGETS pdf-cli pdf-editor pdf-debugger
            RUNTIME DESTINATION bin
            COMPONENT applications)
endif ()
