add_library(util STATIC
        util/allocator.cpp
        util/process_linux.cpp
        util/process_win.cpp)
target_include_directories(util PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(util PRIVATE spdlog::spdlog)

install(TARGETS util EXPORT util ARCHIVE)
install(EXPORT util DESTINATION lib)
