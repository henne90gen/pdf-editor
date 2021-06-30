#include <gtest/gtest.h>

#include <Parser.h>

TEST(Parser, ArrayEmpty) {
    auto textProvider = StringTextProvider("[]");
    auto lexer        = Lexer(textProvider);
    auto parser       = Parser(lexer);
    auto result       = parser.parse();
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->type, Object::Type::ARRAY);
    auto array = (Array *)result;
    ASSERT_EQ(array->values.size(), 0);
}

TEST(Parser, ArrayWithValues) {
    auto textProvider = StringTextProvider("[549 3.14 false /SomeName]");
    auto lexer        = Lexer(textProvider);
    auto parser       = Parser(lexer);
    auto result       = parser.parse();
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->type, Object::Type::ARRAY);
    auto array = (Array *)result;
    ASSERT_EQ(array->values.size(), 4);
}

TEST(Parser, ArrayNested) {
    auto textProvider = StringTextProvider("[549 [3.14 false] /SomeName]");
    auto lexer        = Lexer(textProvider);
    auto parser       = Parser(lexer);
    auto result       = parser.parse();
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->type, Object::Type::ARRAY);
    auto array = (Array *)result;
    ASSERT_EQ(array->values.size(), 3);
}
