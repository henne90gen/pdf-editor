#include <gtest/gtest.h>

#include <pdf_lexer.h>

void assertNextToken(pdf::Lexer &lexer, pdf::Token::Type type, const std::string &content) {
    auto result = lexer.getToken();
    ASSERT_TRUE(result.has_value());
    auto value = result.value();
    ASSERT_EQ(value.type, type);
    ASSERT_EQ(value.content, content);
}

TEST(Lexer, Boolean) {
    auto textProvider = pdf::StringTextProvider("true false");
    auto lexer        = pdf::Lexer(textProvider);
    assertNextToken(lexer, pdf::Token::Type::BOOLEAN, "true");
    assertNextToken(lexer, pdf::Token::Type::BOOLEAN, "false");
}

TEST(Lexer, Integer) {
    auto textProvider = pdf::StringTextProvider("1 -4 +3");
    auto lexer        = pdf::Lexer(textProvider);

    assertNextToken(lexer, pdf::Token::Type::INTEGER, "1");
    assertNextToken(lexer, pdf::Token::Type::INTEGER, "-4");
    assertNextToken(lexer, pdf::Token::Type::INTEGER, "+3");
}

TEST(Lexer, Real) {
    auto textProvider = pdf::StringTextProvider("1.3 -4.2 +3.5");
    auto lexer        = pdf::Lexer(textProvider);

    assertNextToken(lexer, pdf::Token::Type::REAL, "1.3");
    assertNextToken(lexer, pdf::Token::Type::REAL, "-4.2");
    assertNextToken(lexer, pdf::Token::Type::REAL, "+3.5");
}

TEST(Lexer, Name) {
    auto textProvider = pdf::StringTextProvider("/Name1 /ASomewhatLongerName /A;Name_With-Various***Characters? "
                                                "/1.2 /$$ /@pattern /.notdef / /NameWithClosingBracket]");
    auto lexer        = pdf::Lexer(textProvider);

    assertNextToken(lexer, pdf::Token::Type::NAME, "/Name1");
    assertNextToken(lexer, pdf::Token::Type::NAME, "/ASomewhatLongerName");
    assertNextToken(lexer, pdf::Token::Type::NAME, "/A;Name_With-Various***Characters?");
    assertNextToken(lexer, pdf::Token::Type::NAME, "/1.2");
    assertNextToken(lexer, pdf::Token::Type::NAME, "/$$");
    assertNextToken(lexer, pdf::Token::Type::NAME, "/@pattern");
    assertNextToken(lexer, pdf::Token::Type::NAME, "/.notdef");
    assertNextToken(lexer, pdf::Token::Type::NAME, "/");
    assertNextToken(lexer, pdf::Token::Type::NAME, "/NameWithClosingBracket");
    assertNextToken(lexer, pdf::Token::Type::ARRAY_END, "]");
}

TEST(Lexer, NameWithHash) {
    auto textProvider = pdf::StringTextProvider("/Adobe#20Green /PANTONE#205757#20CV /paired#28#29parentheses "
                                                "/The_Key_of_F#23_Minor /A#42");
    auto lexer        = pdf::Lexer(textProvider);

    assertNextToken(lexer, pdf::Token::Type::NAME, "/Adobe#20Green");
    assertNextToken(lexer, pdf::Token::Type::NAME, "/PANTONE#205757#20CV");
    assertNextToken(lexer, pdf::Token::Type::NAME, "/paired#28#29parentheses");
    assertNextToken(lexer, pdf::Token::Type::NAME, "/The_Key_of_F#23_Minor");
    assertNextToken(lexer, pdf::Token::Type::NAME, "/A#42");
}

TEST(Lexer, ArrayAndDict) {
    auto textProvider = pdf::StringTextProvider("[ ] << >>");
    auto lexer        = pdf::Lexer(textProvider);

    assertNextToken(lexer, pdf::Token::Type::ARRAY_START, "[");
    assertNextToken(lexer, pdf::Token::Type::ARRAY_END, "]");
    assertNextToken(lexer, pdf::Token::Type::DICTIONARY_START, "<<");
    assertNextToken(lexer, pdf::Token::Type::DICTIONARY_END, ">>");
}

TEST(Lexer, NewLine) {
    auto textProvider = pdf::StringTextProvider("\n");
    auto lexer        = pdf::Lexer(textProvider);

    assertNextToken(lexer, pdf::Token::Type::NEW_LINE, "\n");
}

TEST(Lexer, IndirectReference) {
    auto textProvider = pdf::StringTextProvider("1 0 R");
    auto lexer        = pdf::Lexer(textProvider);
    assertNextToken(lexer, pdf::Token::Type::INDIRECT_REFERENCE, "1 0 R");
}

TEST(Lexer, HexadecimalString) {
    auto textProvider = pdf::StringTextProvider("<949FFBA879E60749D38B89A33E0DD9E7>");
    auto lexer        = pdf::Lexer(textProvider);
    assertNextToken(lexer, pdf::Token::Type::HEXADECIMAL_STRING, "<949FFBA879E60749D38B89A33E0DD9E7>");
}

TEST(Lexer, IndirectObject) {
    auto textProvider = pdf::StringTextProvider("12 0 obj endobj");
    auto lexer        = pdf::Lexer(textProvider);
    assertNextToken(lexer, pdf::Token::Type::OBJECT_START, "12 0 obj");
    assertNextToken(lexer, pdf::Token::Type::OBJECT_END, "endobj");
}

TEST(Lexer, Stream) {
    auto textProvider = pdf::StringTextProvider("stream\n"
                                                "some bytes\n"
                                                "endstream");
    auto lexer        = pdf::Lexer(textProvider);
    assertNextToken(lexer, pdf::Token::Type::STREAM_START, "stream");
    assertNextToken(lexer, pdf::Token::Type::NEW_LINE, "\n");
    ASSERT_EQ(lexer.advanceStream(10), "some bytes");
    assertNextToken(lexer, pdf::Token::Type::NEW_LINE, "\n");
    assertNextToken(lexer, pdf::Token::Type::STREAM_END, "endstream");
}

TEST(Lexer, DictionaryStream) {
    auto textProvider = pdf::StringTextProvider("<</Length 45/Filter/FlateDecode>>");
    auto lexer        = pdf::Lexer(textProvider);
    assertNextToken(lexer, pdf::Token::Type::DICTIONARY_START, "<<");

    assertNextToken(lexer, pdf::Token::Type::NAME, "/Length");
    assertNextToken(lexer, pdf::Token::Type::INTEGER, "45");

    assertNextToken(lexer, pdf::Token::Type::NAME, "/Filter");
    assertNextToken(lexer, pdf::Token::Type::NAME, "/FlateDecode");

    assertNextToken(lexer, pdf::Token::Type::DICTIONARY_END, ">>");
}

TEST(Lexer, Null) {
    auto textProvider = pdf::StringTextProvider("null");
    auto lexer        = pdf::Lexer(textProvider);
    assertNextToken(lexer, pdf::Token::Type::NULL_OBJ, "null");
}

TEST(Lexer, LiteralString) {
    auto textProvider = pdf::StringTextProvider("(This is a string)"
                                                "(Strings may contain newlines\nand such.)"
                                                "(Strings may contain balanced parentheses ( ) and\nspecial characters (*!&}^% and so on).)"
                                                "(The following is an empty string.)"
                                                "()"
                                                "(It has zero (0) length.)");
    auto lexer        = pdf::Lexer(textProvider);
    assertNextToken(lexer, pdf::Token::Type::LITERAL_STRING, "(This is a string)");
}
