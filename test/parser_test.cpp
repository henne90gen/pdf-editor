#include <gtest/gtest.h>

#include <Parser.h>

template <typename T> void assertParses(const std::string &input, std::function<void(T *)> func) {
    auto textProvider = StringTextProvider(input);
    auto lexer        = Lexer(textProvider);
    auto parser       = Parser(lexer);
    auto result       = parser.parse();
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->type, T::staticType());
    func(result->as<T>());
}

TEST(Parser, Boolean) {
    assertParses<Boolean>("true", [](auto *result) {
        ASSERT_EQ(result->value, true); //
    });
}

TEST(Parser, Integer) {
    assertParses<Integer>("123", [](auto *result) {
        ASSERT_EQ(result->value, 123); //
    });
}

TEST(Parser, Real) {
    assertParses<Real>("12.3", [](auto *result) {
        ASSERT_EQ(result->value, 12.3); //
    });
}

TEST(Parser, Name) {
    assertParses<Name>("/SomeName", [](auto *result) {
        ASSERT_EQ(result->value, "SomeName"); //
    });
}

TEST(Parser, ArrayEmpty) {
    assertParses<Array>("[]", [](auto *result) {
        ASSERT_EQ(result->values.size(), 0); //
    });
}

TEST(Parser, ArrayWithValues) {
    assertParses<Array>("[549 3.14 false /SomeName]", [](auto *result) {
        ASSERT_EQ(result->values.size(), 4); //
    });
}

TEST(Parser, ArrayNested) {
    assertParses<Array>("[549 [3.14 false] /SomeName]", [](auto *result) {
        ASSERT_EQ(result->values.size(), 3); //
    });
}
