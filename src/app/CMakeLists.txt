
#function(embed_file file generated_c)
#    get_filename_component(file_name ${file} NAME)
#    set(c_file ${CMAKE_CURRENT_BINARY_DIR}/${file_name}.c)
#    add_custom_command(
#            OUTPUT ${c_file} ${CMAKE_CURRENT_BINARY_DIR}/${file_name}.h
#            COMMAND python3 ${CMAKE_SOURCE_DIR}/scripts/embed.py ${CMAKE_CURRENT_SOURCE_DIR}/${file} ${CMAKE_CURRENT_BINARY_DIR}/${file_name}
#            DEPENDS ${file})
#    set(${generated_c} ${c_file} PARENT_SCOPE)
#endfunction()

add_subdirectory(cli)
add_subdirectory(common)
add_subdirectory(debugger)
add_subdirectory(editor)

#install(TARGETS pdf-cli pdf-editor pdf-debugger
#        RUNTIME DESTINATION bin)
