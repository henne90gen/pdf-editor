add_library(pdf-lib pdf_reader.cpp Lexer.cpp Parser.cpp)
target_include_directories(pdf-lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
