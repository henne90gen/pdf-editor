find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtkmm-3.0)

add_executable(pdf-app MainWindow.cpp PdfPage.cpp)
target_link_libraries(pdf-app ${GTK3_LIBRARIES} pdf-lib)
target_link_options(pdf-app PRIVATE ${GTK3_LDFLAGS_OTHER})
target_compile_options(pdf-app PRIVATE ${GTK3_CFLAGS_OTHER})
target_link_directories(pdf-app PRIVATE ${GTK3_LIBRARY_DIRS})
target_include_directories(pdf-app PRIVATE ${GTK3_INCLUDE_DIRS})
