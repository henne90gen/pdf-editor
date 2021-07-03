add_library(pdf-lib pdf_reader.cpp pdf_lexer.cpp pdf_parser.cpp pdf_file.cpp pdf_objects.cpp)
target_include_directories(pdf-lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
