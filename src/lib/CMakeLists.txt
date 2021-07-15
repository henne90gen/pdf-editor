add_library(pdf-lib
        pdf/lexer.cpp
        pdf/parser.cpp
        pdf/document.cpp
        pdf/objects.cpp
        pdf/graphics.cpp
        pdf/text.cpp
        pdf/operator_parser.cpp
        pdf/renderer.cpp)
target_include_directories(pdf-lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(pdf-lib PUBLIC zlibstatic)
