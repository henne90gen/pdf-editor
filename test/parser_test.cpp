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

TEST(Parser, DictionaryEmpty) {
    assertParses<Dictionary>("<< >>", [](Dictionary *result) {
        ASSERT_EQ(result->values.size(), 0); //
    });
}

TEST(Parser, DictionaryWithValues) {
    assertParses<Dictionary>("<< /Type /Hello \n /Key /Value \n >>", [](Dictionary *result) {
        ASSERT_EQ(result->values.size(), 2); //
    });
}

TEST(Parser, DictionaryNested) {
    assertParses<Dictionary>("<< /Type /Hello \n /Dict << /Key /Value \n >> \n >>", [](Dictionary *result) {
        ASSERT_EQ(result->values.size(), 2); //
    });
}

TEST(Parser, DictionaryTrailer) {
    assertParses<Dictionary>("<</Size 9/Root 7 0 R\n"
                             "/Info 8 0 R\n"
                             "/ID [ <949FFBA879E60749D38B89A33E0DD9E7>\n"
                             "<949FFBA879E60749D38B89A33E0DD9E7> ]\n"
                             "/DocChecksum /87E47BA8C63C8BA796458FA05DBE8C32\n"
                             ">>",
                             [](Dictionary *result) {
                                 ASSERT_EQ(result->values.size(), 5); //
                             });
}