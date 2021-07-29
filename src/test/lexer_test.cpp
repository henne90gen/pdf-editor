#include <gtest/gtest.h>

#include <pdf/lexer.h>

void assertNextToken(pdf::Lexer &lexer, pdf::Token::Type type, const std::string &content) {
    auto result = lexer.getToken();
    ASSERT_TRUE(result.has_value());
    auto value = result.value();
    ASSERT_EQ(value.type, type);
    ASSERT_EQ(value.content, content);
}

#define assertNoMoreTokens(lexer)                                                                                      \
    auto result = (lexer).getToken();                                                                                  \
    ASSERT_FALSE(result.has_value()) << result.value().content

TEST(Lexer, Boolean) {
    auto textProvider = pdf::StringTextProvider("true false");
    auto lexer        = pdf::TextLexer(textProvider);
    assertNextToken(lexer, pdf::Token::Type::BOOLEAN, "true");
    assertNextToken(lexer, pdf::Token::Type::BOOLEAN, "false");
    assertNoMoreTokens(lexer);
}

TEST(Lexer, Integer) {
    auto textProvider = pdf::StringTextProvider("1 -4 +3");
    auto lexer        = pdf::TextLexer(textProvider);
    assertNextToken(lexer, pdf::Token::Type::INTEGER, "1");
    assertNextToken(lexer, pdf::Token::Type::INTEGER, "-4");
    assertNextToken(lexer, pdf::Token::Type::INTEGER, "+3");
    assertNoMoreTokens(lexer);
}

TEST(Lexer, Real) {
    auto textProvider = pdf::StringTextProvider("1.3 -4.2 +3.5");
    auto lexer        = pdf::TextLexer(textProvider);
    assertNextToken(lexer, pdf::Token::Type::REAL, "1.3");
    assertNextToken(lexer, pdf::Token::Type::REAL, "-4.2");
    assertNextToken(lexer, pdf::Token::Type::REAL, "+3.5");
    assertNoMoreTokens(lexer);
}

TEST(Lexer, Name) {
    auto textProvider = pdf::StringTextProvider("/Name1 /ASomewhatLongerName /A;Name_With-Various***Characters? "
                                                "/1.2 /$$ /@pattern /.notdef / /NameWithClosingBracket]");
    auto lexer        = pdf::TextLexer(textProvider);
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
    assertNoMoreTokens(lexer);
}

TEST(Lexer, NameWithHash) {
    auto textProvider = pdf::StringTextProvider("/Adobe#20Green /PANTONE#205757#20CV /paired#28#29parentheses "
                                                "/The_Key_of_F#23_Minor /A#42");
    auto lexer        = pdf::TextLexer(textProvider);
    assertNextToken(lexer, pdf::Token::Type::NAME, "/Adobe#20Green");
    assertNextToken(lexer, pdf::Token::Type::NAME, "/PANTONE#205757#20CV");
    assertNextToken(lexer, pdf::Token::Type::NAME, "/paired#28#29parentheses");
    assertNextToken(lexer, pdf::Token::Type::NAME, "/The_Key_of_F#23_Minor");
    assertNextToken(lexer, pdf::Token::Type::NAME, "/A#42");
    assertNoMoreTokens(lexer);
}

TEST(Lexer, ArrayAndDict) {
    auto textProvider = pdf::StringTextProvider("[ ] << >>");
    auto lexer        = pdf::TextLexer(textProvider);
    assertNextToken(lexer, pdf::Token::Type::ARRAY_START, "[");
    assertNextToken(lexer, pdf::Token::Type::ARRAY_END, "]");
    assertNextToken(lexer, pdf::Token::Type::DICTIONARY_START, "<<");
    assertNextToken(lexer, pdf::Token::Type::DICTIONARY_END, ">>");
    assertNoMoreTokens(lexer);
}

TEST(Lexer, NewLine) {
    auto textProvider = pdf::StringTextProvider("\n");
    auto lexer        = pdf::TextLexer(textProvider);
    assertNextToken(lexer, pdf::Token::Type::NEW_LINE, "\n");
    assertNoMoreTokens(lexer);
}

TEST(Lexer, IndirectReference) {
    auto textProvider = pdf::StringTextProvider("1 0 R");
    auto lexer        = pdf::TextLexer(textProvider);
    assertNextToken(lexer, pdf::Token::Type::INDIRECT_REFERENCE, "1 0 R");
    assertNoMoreTokens(lexer);
}

TEST(Lexer, HexadecimalString) {
    auto textProvider = pdf::StringTextProvider("<949FFBA879E60749D38B89A33E0DD9E7>");
    auto lexer        = pdf::TextLexer(textProvider);
    assertNextToken(lexer, pdf::Token::Type::HEXADECIMAL_STRING, "<949FFBA879E60749D38B89A33E0DD9E7>");
    assertNoMoreTokens(lexer);
}

TEST(Lexer, IndirectObject) {
    auto textProvider = pdf::StringTextProvider("12 0 obj endobj");
    auto lexer        = pdf::TextLexer(textProvider);
    assertNextToken(lexer, pdf::Token::Type::OBJECT_START, "12 0 obj");
    assertNextToken(lexer, pdf::Token::Type::OBJECT_END, "endobj");
    assertNoMoreTokens(lexer);
}

TEST(Lexer, Stream) {
    auto textProvider = pdf::StringTextProvider("stream\n"
                                                "some bytes\n"
                                                "endstream");
    auto lexer        = pdf::TextLexer(textProvider);
    assertNextToken(lexer, pdf::Token::Type::STREAM_START, "stream");
    assertNextToken(lexer, pdf::Token::Type::NEW_LINE, "\n");
    ASSERT_EQ(lexer.advanceStream(10), "some bytes");
    assertNextToken(lexer, pdf::Token::Type::NEW_LINE, "\n");
    assertNextToken(lexer, pdf::Token::Type::STREAM_END, "endstream");
    assertNoMoreTokens(lexer);
}

TEST(Lexer, DictionaryStream) {
    auto textProvider = pdf::StringTextProvider("<</Length 45/Filter/FlateDecode>>");
    auto lexer        = pdf::TextLexer(textProvider);
    assertNextToken(lexer, pdf::Token::Type::DICTIONARY_START, "<<");
    assertNextToken(lexer, pdf::Token::Type::NAME, "/Length");
    assertNextToken(lexer, pdf::Token::Type::INTEGER, "45");

    assertNextToken(lexer, pdf::Token::Type::NAME, "/Filter");
    assertNextToken(lexer, pdf::Token::Type::NAME, "/FlateDecode");

    assertNextToken(lexer, pdf::Token::Type::DICTIONARY_END, ">>");
    assertNoMoreTokens(lexer);
}

TEST(Lexer, Null) {
    auto textProvider = pdf::StringTextProvider("null");
    auto lexer        = pdf::TextLexer(textProvider);
    assertNextToken(lexer, pdf::Token::Type::NULL_OBJ, "null");
    assertNoMoreTokens(lexer);
}

TEST(Lexer, LiteralString) {
    auto textProvider = pdf::StringTextProvider(
          "(This is a string)"
          "(Strings may contain newlines\nand such.)"
          "(Strings may contain balanced parentheses ( ) and\nspecial characters (*!&}^% and so on).)"
          "(The following is an empty string.)"
          "()"
          "(It has zero (0) length.)");
    auto lexer = pdf::TextLexer(textProvider);
    assertNextToken(lexer, pdf::Token::Type::LITERAL_STRING, "(This is a string)");
    assertNextToken(lexer, pdf::Token::Type::LITERAL_STRING, "(Strings may contain newlines\nand such.)");
    assertNextToken(lexer, pdf::Token::Type::LITERAL_STRING,
                    "(Strings may contain balanced parentheses ( ) and\nspecial characters (*!&}^% and so on).)");
    assertNextToken(lexer, pdf::Token::Type::LITERAL_STRING, "(The following is an empty string.)");
    assertNextToken(lexer, pdf::Token::Type::LITERAL_STRING, "()");
    assertNextToken(lexer, pdf::Token::Type::LITERAL_STRING, "(It has zero (0) length.)");
    assertNoMoreTokens(lexer);
}

TEST(Lexer, TextOperators) {
    auto textProvider = pdf::StringTextProvider("BT ET Td TD Tm T* Tc Tw Tz TL Tf Tr Ts");
    auto lexer        = pdf::TextLexer(textProvider);
    assertNextToken(lexer, pdf::Token::Type::OPERATOR, "BT");
    assertNextToken(lexer, pdf::Token::Type::OPERATOR, "ET");
    assertNextToken(lexer, pdf::Token::Type::OPERATOR, "Td");
    assertNextToken(lexer, pdf::Token::Type::OPERATOR, "TD");
    assertNextToken(lexer, pdf::Token::Type::OPERATOR, "Tm");
    assertNextToken(lexer, pdf::Token::Type::OPERATOR, "T*");
    assertNextToken(lexer, pdf::Token::Type::OPERATOR, "Tc");
    assertNextToken(lexer, pdf::Token::Type::OPERATOR, "Tw");
    assertNextToken(lexer, pdf::Token::Type::OPERATOR, "Tz");
    assertNextToken(lexer, pdf::Token::Type::OPERATOR, "TL");
    assertNextToken(lexer, pdf::Token::Type::OPERATOR, "Tf");
    assertNextToken(lexer, pdf::Token::Type::OPERATOR, "Tr");
    assertNextToken(lexer, pdf::Token::Type::OPERATOR, "Ts");
    assertNoMoreTokens(lexer);
}

TEST(Lexer, GraphicsOperators) {
    auto textProvider = pdf::StringTextProvider("q Q cm w J j M d ri i gs");
    auto lexer        = pdf::TextLexer(textProvider);
    assertNextToken(lexer, pdf::Token::Type::OPERATOR, "q");
    assertNextToken(lexer, pdf::Token::Type::OPERATOR, "Q");
    assertNextToken(lexer, pdf::Token::Type::OPERATOR, "cm");
    assertNextToken(lexer, pdf::Token::Type::OPERATOR, "w");
    assertNextToken(lexer, pdf::Token::Type::OPERATOR, "J");
    assertNextToken(lexer, pdf::Token::Type::OPERATOR, "j");
    assertNextToken(lexer, pdf::Token::Type::OPERATOR, "M");
    assertNextToken(lexer, pdf::Token::Type::OPERATOR, "d");
    assertNextToken(lexer, pdf::Token::Type::OPERATOR, "ri");
    assertNextToken(lexer, pdf::Token::Type::OPERATOR, "i");
    assertNextToken(lexer, pdf::Token::Type::OPERATOR, "gs");
    assertNoMoreTokens(lexer);
}

TEST(Lexer, PathContructionOperators) {
    auto textProvider = pdf::StringTextProvider("m l c v y h re");
    auto lexer        = pdf::TextLexer(textProvider);
    assertNextToken(lexer, pdf::Token::Type::OPERATOR, "m");
    assertNextToken(lexer, pdf::Token::Type::OPERATOR, "l");
    assertNextToken(lexer, pdf::Token::Type::OPERATOR, "c");
    assertNextToken(lexer, pdf::Token::Type::OPERATOR, "v");
    assertNextToken(lexer, pdf::Token::Type::OPERATOR, "y");
    assertNextToken(lexer, pdf::Token::Type::OPERATOR, "h");
    assertNextToken(lexer, pdf::Token::Type::OPERATOR, "re");
    assertNoMoreTokens(lexer);
}

TEST(Lexer, PathPaintingOperators) {
    auto textProvider = pdf::StringTextProvider("S s f* F f B* B b* b n");
    auto lexer        = pdf::TextLexer(textProvider);
    assertNextToken(lexer, pdf::Token::Type::OPERATOR, "S");
    assertNextToken(lexer, pdf::Token::Type::OPERATOR, "s");
    assertNextToken(lexer, pdf::Token::Type::OPERATOR, "f*");
    assertNextToken(lexer, pdf::Token::Type::OPERATOR, "F");
    assertNextToken(lexer, pdf::Token::Type::OPERATOR, "f");
    assertNextToken(lexer, pdf::Token::Type::OPERATOR, "B*");
    assertNextToken(lexer, pdf::Token::Type::OPERATOR, "B");
    assertNextToken(lexer, pdf::Token::Type::OPERATOR, "b*");
    assertNextToken(lexer, pdf::Token::Type::OPERATOR, "b");
    assertNextToken(lexer, pdf::Token::Type::OPERATOR, "n");
    assertNoMoreTokens(lexer);
}

TEST(Lexer, CMap) {
    auto textProvider = pdf::StringTextProvider("begincmap endcmap usecmap usefont begincodespacerange "
                                                "endcodespacerange beginbfchar endbfchar beginbfrange endbfrange "
                                                "begincidchar endcidchar begincidrange endcidrange beginnotdefchar "
                                                "endnotdefchar beginnotdefrange endnotdefrange");
    auto lexer        = pdf::TextLexer(textProvider);
    assertNextToken(lexer, pdf::Token::Type::CMAP_BEGIN, "begincmap");
    assertNextToken(lexer, pdf::Token::Type::CMAP_END, "endcmap");
    assertNextToken(lexer, pdf::Token::Type::CMAP_USE, "usecmap");
    assertNextToken(lexer, pdf::Token::Type::CMAP_USE_FONT, "usefont");
    assertNextToken(lexer, pdf::Token::Type::CMAP_BEGIN_CODE_SPACE_RANGE, "begincodespacerange");
    assertNextToken(lexer, pdf::Token::Type::CMAP_END_CODE_SPACE_RANGE, "endcodespacerange");
    assertNextToken(lexer, pdf::Token::Type::CMAP_BEGIN_BF_CHAR, "beginbfchar");
    assertNextToken(lexer, pdf::Token::Type::CMAP_END_BF_CHAR, "endbfchar");
    assertNextToken(lexer, pdf::Token::Type::CMAP_BEGIN_BF_RANGE, "beginbfrange");
    assertNextToken(lexer, pdf::Token::Type::CMAP_END_BF_RANGE, "endbfrange");
    assertNextToken(lexer, pdf::Token::Type::CMAP_BEGIN_CID_CHAR, "begincidchar");
    assertNextToken(lexer, pdf::Token::Type::CMAP_END_CID_CHAR, "endcidchar");
    assertNextToken(lexer, pdf::Token::Type::CMAP_BEGIN_CID_RANGE, "begincidrange");
    assertNextToken(lexer, pdf::Token::Type::CMAP_END_CID_RANGE, "endcidrange");
    assertNextToken(lexer, pdf::Token::Type::CMAP_BEGIN_NOTDEF_CHAR, "beginnotdefchar");
    assertNextToken(lexer, pdf::Token::Type::CMAP_END_NOTDEF_CHAR, "endnotdefchar");
    assertNextToken(lexer, pdf::Token::Type::CMAP_BEGIN_NOTDEF_RANGE, "beginnotdefrange");
    assertNextToken(lexer, pdf::Token::Type::CMAP_END_NOTDEF_RANGE, "endnotdefrange");
    assertNoMoreTokens(lexer);
}
