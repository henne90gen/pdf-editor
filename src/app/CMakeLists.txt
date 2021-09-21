
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtkmm-3.0)

function(target_link_gtk3 target)
    target_link_libraries(${target} PRIVATE ${GTK3_LIBRARIES})
    target_link_options(${target} PRIVATE ${GTK3_LDFLAGS_OTHER})
    target_compile_options(${target} PRIVATE ${GTK3_CFLAGS_OTHER})
    target_link_directories(${target} PRIVATE ${GTK3_LIBRARY_DIRS})
    target_include_directories(${target} PRIVATE ${GTK3_INCLUDE_DIRS})
endfunction()

add_executable(pdf-app MainWindow.cpp PdfPage.cpp)
target_link_gtk3(pdf-app)
target_link_libraries(pdf-app PRIVATE pdf-lib spdlog::spdlog)

add_subdirectory(gtk-examples)

install(TARGETS pdf-app DESTINATION bin)
