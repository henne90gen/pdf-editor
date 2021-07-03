#include <gtest/gtest.h>

#include <pdf_parser.h>

template <typename T> void assertParses(const std::string &input, std::function<void(T *)> func) {
    auto textProvider = pdf::StringTextProvider(input);
    auto lexer        = pdf::Lexer(textProvider);
    auto parser       = pdf::Parser(lexer);
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
        ASSERT_EQ(result->values["Type"]->as<Name>()->value, "Hello");
        ASSERT_EQ(result->values["Key"]->as<Name>()->value, "Value");
    });
}

TEST(Parser, DictionaryNested) {
    assertParses<Dictionary>("<< /Type /Hello \n /Dict << /Key /Value \n >> \n >>", [](Dictionary *result) {
        ASSERT_EQ(result->values.size(), 2);
        ASSERT_EQ(result->values["Type"]->as<Name>()->value, "Hello");
        ASSERT_EQ(result->values["Dict"]->as<Dictionary>()->values.size(), 1);
    });
}

TEST(Parser, IndirectReference) {
    assertParses<IndirectReference>("1 2 R", [](IndirectReference *result) {
        ASSERT_EQ(result->objectNumber, 1);
        ASSERT_EQ(result->generationNumber, 2);
    });
}

TEST(Parser, HexadecimalString) {
    assertParses<HexadecimalString>("<949FFBA879E60749D38B89A33E0DD9E7>", [](HexadecimalString *result) {
        ASSERT_EQ(result->value, "949FFBA879E60749D38B89A33E0DD9E7"); //
    });
}

TEST(Parser, DictionaryTrailer) {
    const std::string input = "<</Size 9/Root 7 0 R\n"
                              "/Info 8 0 R\n"
                              "/ID [ <949FFBA879E60749D38B89A33E0DD9E7>\n"
                              "<949FFBA879E60749D38B89A33E0DD9E7> ]\n"
                              "/DocChecksum /87E47BA8C63C8BA796458FA05DBE8C32\n"
                              ">>";
    assertParses<Dictionary>(input, [](Dictionary *result) {
        ASSERT_EQ(result->values.size(), 5); //
        ASSERT_EQ(result->values["Size"]->as<Integer>()->value, 9);
        ASSERT_EQ(result->values["Info"]->as<IndirectReference>()->objectNumber, 8);
        ASSERT_EQ(result->values["Info"]->as<IndirectReference>()->generationNumber, 0);
        ASSERT_EQ(result->values["ID"]->as<Array>()->values.size(), 2);
        ASSERT_EQ(result->values["ID"]->as<Array>()->values[0]->as<HexadecimalString>()->value,
                  "949FFBA879E60749D38B89A33E0DD9E7");
        ASSERT_EQ(result->values["ID"]->as<Array>()->values[1]->as<HexadecimalString>()->value,
                  "949FFBA879E60749D38B89A33E0DD9E7");
        ASSERT_EQ(result->values["DocChecksum"]->as<Name>()->value, "87E47BA8C63C8BA796458FA05DBE8C32");
    });
}

TEST(Parser, DictionaryStream) {
    assertParses<Dictionary>("<</Length 45/Filter/FlateDecode>>",
                             [](Dictionary *result) { ASSERT_EQ(result->values.size(), 2); });
}

TEST(Parser, MoreThanOneObject) {
    assertParses<Array>("[/MyName] /AnotherName", [](Array *result) {
        ASSERT_EQ(result->values.size(), 1); //
    });
}

TEST(Parser, IndirectObject) {
    assertParses<IndirectObject>("12 1 obj \n /MyName \n endobj", [](IndirectObject *result) {
        ASSERT_EQ(result->objectNumber, 12);
        ASSERT_EQ(result->generationNumber, 1);
        ASSERT_EQ(result->object->as<Name>()->value, "MyName");
    });
}

TEST(Parser, Stream) {
    const std::string input = "<</Length 10/Filter/FlateDecode>>\n"
                              "stream\n"
                              "some bytes\n"
                              "endstream";
    assertParses<Stream>(input, [](Stream *result) {
        ASSERT_EQ(result->length, 10);
        ASSERT_EQ(std::string(result->data, result->length), "some bytes");
    });
}
