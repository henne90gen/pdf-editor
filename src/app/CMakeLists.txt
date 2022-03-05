
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED gtkmm-4.0>=4.4.0)

function(target_link_gtk4 target)
    target_link_libraries(${target} PRIVATE ${GTK4_LIBRARIES})
    target_link_options(${target} PRIVATE ${GTK4_LDFLAGS_OTHER})
    target_compile_options(${target} PRIVATE ${GTK4_CFLAGS_OTHER})
    target_link_directories(${target} PRIVATE ${GTK4_LIBRARY_DIRS})
    target_include_directories(${target} PRIVATE ${GTK4_INCLUDE_DIRS})
endfunction()

add_subdirectory(cli)
add_subdirectory(common)
add_subdirectory(debugger)
add_subdirectory(editor)

get_filename_component(COMPILER_PATH ${CMAKE_CXX_COMPILER} PATH)
# GLib Schemas
add_custom_target(copy_glib_schemas
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${COMPILER_PATH}/../share/glib-2.0/schemas
        ${CMAKE_CURRENT_BINARY_DIR}/share/glib-2.0/schemas)
add_custom_target(compile_glib_schemas
        COMMAND glib-compile-schemas ${CMAKE_CURRENT_BINARY_DIR}/share/glib-2.0/schemas)
add_dependencies(compile_glib_schemas copy_glib_schemas)
add_dependencies(pdf-editor compile_glib_schemas)
add_dependencies(pdf-debugger compile_glib_schemas)

if (WIN32)
        install(TARGETS pdf-cli pdf-editor pdf-debugger
                RUNTIME_DEPENDENCIES
                DIRECTORIES $<$<PLATFORM_ID:Windows>:${COMPILER_PATH}> ${COMPILER_PATH}/../lib
                PRE_EXCLUDE_REGEXES "api-ms-" "ext-ms-" "libc.so.6"
                POST_EXCLUDE_REGEXES ".*system32/.*\\.dll" >
                RUNTIME DESTINATION bin
                COMPONENT applications)
        install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/share/glib-2.0/schemas
                DESTINATION share/glib-2.0
                COMPONENT applications)
        install(DIRECTORY ${COMPILER_PATH}/../share/icons
                DESTINATION share
                COMPONENT applications)
else ()
        install(TARGETS pdf-cli pdf-editor pdf-debugger
                RUNTIME DESTINATION bin
                COMPONENT applications)
endif()
