#include <gtest/gtest.h>

#include <Lexer.h>

void assertNextToken(Lexer &lexer, Token::Type type, const std::string &content) {
    auto result = lexer.getToken();
    ASSERT_TRUE(result.has_value());
    auto value = result.value();
    ASSERT_EQ(value.type, type);
    ASSERT_EQ(value.content, content);
}

TEST(Lexer, Boolean) {
    auto textProvider = StringTextProvider("true false");
    auto lexer        = Lexer(textProvider);
    assertNextToken(lexer, Token::Type::BOOLEAN, "true");
    assertNextToken(lexer, Token::Type::BOOLEAN, "false");
}

TEST(Lexer, Integer) {
    auto textProvider = StringTextProvider("1 -4 +3");
    auto lexer        = Lexer(textProvider);

    assertNextToken(lexer, Token::Type::INTEGER, "1");
    assertNextToken(lexer, Token::Type::INTEGER, "-4");
    assertNextToken(lexer, Token::Type::INTEGER, "+3");
}

TEST(Lexer, Real) {
    auto textProvider = StringTextProvider("1.3 -4.2 +3.5");
    auto lexer        = Lexer(textProvider);

    assertNextToken(lexer, Token::Type::REAL, "1.3");
    assertNextToken(lexer, Token::Type::REAL, "-4.2");
    assertNextToken(lexer, Token::Type::REAL, "+3.5");
}

TEST(Lexer, Name) {
    auto textProvider = StringTextProvider("/Name1 /ASomewhatLongerName /A;Name_With-Various***Characters? "
                                           "/1.2 /$$ /@pattern /.notdef / /NameWithClosingBracket]");
    auto lexer        = Lexer(textProvider);

    assertNextToken(lexer, Token::Type::NAME, "/Name1");
    assertNextToken(lexer, Token::Type::NAME, "/ASomewhatLongerName");
    assertNextToken(lexer, Token::Type::NAME, "/A;Name_With-Various***Characters?");
    assertNextToken(lexer, Token::Type::NAME, "/1.2");
    assertNextToken(lexer, Token::Type::NAME, "/$$");
    assertNextToken(lexer, Token::Type::NAME, "/@pattern");
    assertNextToken(lexer, Token::Type::NAME, "/.notdef");
    assertNextToken(lexer, Token::Type::NAME, "/");
    assertNextToken(lexer, Token::Type::NAME, "/NameWithClosingBracket");
    assertNextToken(lexer, Token::Type::ARRAY_END, "]");
}

TEST(Lexer, NameWithHash) {
    auto textProvider = StringTextProvider("/Adobe#20Green /PANTONE#205757#20CV /paired#28#29parentheses "
                                           "/The_Key_of_F#23_Minor /A#42");
    auto lexer        = Lexer(textProvider);

    assertNextToken(lexer, Token::Type::NAME, "/Adobe#20Green");
    assertNextToken(lexer, Token::Type::NAME, "/PANTONE#205757#20CV");
    assertNextToken(lexer, Token::Type::NAME, "/paired#28#29parentheses");
    assertNextToken(lexer, Token::Type::NAME, "/The_Key_of_F#23_Minor");
    assertNextToken(lexer, Token::Type::NAME, "/A#42");
}

TEST(Lexer, ArrayAndDict) {
    auto textProvider = StringTextProvider("[ ] << >>");
    auto lexer        = Lexer(textProvider);

    assertNextToken(lexer, Token::Type::ARRAY_START, "[");
    assertNextToken(lexer, Token::Type::ARRAY_END, "]");
    assertNextToken(lexer, Token::Type::DICTIONARY_START, "<<");
    assertNextToken(lexer, Token::Type::DICTIONARY_END, ">>");
}

TEST(Lexer, NewLine) {
    auto textProvider = StringTextProvider("\n");
    auto lexer        = Lexer(textProvider);

    assertNextToken(lexer, Token::Type::NEW_LINE, "\n");
}

TEST(Lexer, IndirectReference) {
    auto textProvider = StringTextProvider("1 0 R");
    auto lexer        = Lexer(textProvider);
    assertNextToken(lexer, Token::Type::INDIRECT_REFERENCE, "1 0 R");
}

TEST(Lexer, HexadecimalString) {
    auto textProvider = StringTextProvider("<949FFBA879E60749D38B89A33E0DD9E7>");
    auto lexer        = Lexer(textProvider);
    assertNextToken(lexer, Token::Type::HEXADECIMAL_STRING, "<949FFBA879E60749D38B89A33E0DD9E7>");
}

TEST(Lexer, IndirectObject) {
    auto textProvider = StringTextProvider("12 0 obj endobj");
    auto lexer        = Lexer(textProvider);
    assertNextToken(lexer, Token::Type::OBJECT_START, "12 0 obj");
    assertNextToken(lexer, Token::Type::OBJECT_END, "endobj");
}

TEST(Lexer, Stream) {
    auto textProvider = StringTextProvider("stream\n"
                                           "x£3╨3T(τ*T0P0╨30▓P034╘│47T0╖ä╨E⌐\\ßZ\n"
                                           "y\\ü\n"
                                           " ╢¼\n"
                                           "endstream");
    auto lexer        = Lexer(textProvider);
    assertNextToken(lexer, Token::Type::STREAM_START, "stream");
    assertNextToken(lexer, Token::Type::NEW_LINE, "\n");
    lexer.advanceStream(45);
    assertNextToken(lexer, Token::Type::NEW_LINE, "\n");
    assertNextToken(lexer, Token::Type::STREAM_END, "endstream");
}
