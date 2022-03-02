
set(CMAKE_INSTALL_RPATH $ORIGIN/../lib)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED
        gtkmm-4.0
        gmodule-2.0
        fribidi
        fontconfig
        epoxy
        xkbcommon
        wayland-client
        wayland-egl
        graphite2
        xcb
        xcb-render
        xcb-shm
        mount
        expat
#        sqlite3
#        json-glib-1.0
#        lzo2
        blkid
        libjpeg
        zlib
        libpng16
        libthai
        libffi
        libpcre
        libbrotlidec
        libbrotlicommon
#        libzstd
        liblzma
        xi
        x11
        xext
        xcursor
        xdamage
        xfixes
        xrandr
        xinerama
        xrender
        xau
        xdmcp
        pangoft2
#        tracker-sparql-3.0
#        cloudproviders
        libtiff-4
        pixman-1
#        bzip2
        icu-uc
        icu-i18n
        libxml-2.0
        datrie-0.2
        )

function(target_link_gtk4 target)
    target_link_libraries(${target} PRIVATE ${GTK4_LIBRARIES} $<$<PLATFORM_ID:Linux>:cairo-script-interpreter stemmer>)
    target_link_options(${target} PRIVATE ${GTK4_LDFLAGS_OTHER})
    target_compile_options(${target} PRIVATE ${GTK4_CFLAGS_OTHER})
    target_link_directories(${target} PRIVATE ${GTK4_LIBRARY_DIRS})
    target_include_directories(${target} PRIVATE ${GTK4_INCLUDE_DIRS})
endfunction()

function(embed_file file generated_c)
    # TODO replace this with "glib-compile-resources tfe3.gresource.xml --target=resources.c --generate-source"
    get_filename_component(file_name ${file} NAME)
    set(c_file ${CMAKE_CURRENT_BINARY_DIR}/${file_name}.cpp)
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

get_filename_component(COMPILER_PATH ${CMAKE_CXX_COMPILER} PATH)
message(STATUS "Compiler path: ${COMPILER_PATH}")
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

set_target_properties(pdf-cli PROPERTIES INSTALL_RPATH "$ORIGIN/../lib")
set_target_properties(pdf-debugger PROPERTIES INSTALL_RPATH "$ORIGIN/../lib")
set_target_properties(pdf-editor PROPERTIES INSTALL_RPATH "$ORIGIN/../lib")
