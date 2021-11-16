#include <gtest/gtest.h>

#include <pdf/parser.h>

class TestReferenceResolver : public pdf::ReferenceResolver {
  public:
    explicit TestReferenceResolver(const std::vector<pdf::IndirectObject *> &refs) : references(refs) {}

    pdf::IndirectObject *resolve(const pdf::IndirectReference *reference) override {
        if (reference->objectNumber >= static_cast<int>(references.size())) {
            return nullptr;
        }
        return references[reference->objectNumber];
    }

  private:
    const std::vector<pdf::IndirectObject *> &references;
};

template <typename T> void assertParses(const std::string &input, std::function<void(T *)> func) {
    auto textProvider = pdf::StringTextProvider(input);
    auto lexer        = pdf::TextLexer(textProvider);
    auto allocator    = pdf::Allocator();
    allocator.init(1000);
    auto parser = pdf::Parser(lexer, allocator);
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->type, T::staticType());
    func(result->as<T>());
}

template <typename T>
void assertParsesWithReferenceResolver(const std::string &input, pdf::ReferenceResolver *referenceResolver,
                                       std::function<void(T *)> func) {
    auto textProvider = pdf::StringTextProvider(input);
    auto lexer        = pdf::TextLexer(textProvider);
    auto allocator    = pdf::Allocator();
    allocator.init(1000);
    auto parser = pdf::Parser(lexer, allocator, referenceResolver);
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->type, T::staticType());
    func(result->as<T>());
}

TEST(Parser, Boolean) {
    assertParses<pdf::Boolean>("true", [](auto *result) {
        ASSERT_EQ(result->value, true); //
    });
}

TEST(Parser, Integer) {
    assertParses<pdf::Integer>("123", [](auto *result) {
        ASSERT_EQ(result->value, 123); //
    });
}

TEST(Parser, Real) {
    assertParses<pdf::Real>("12.3", [](auto *result) {
        ASSERT_EQ(result->value, 12.3); //
    });
}

TEST(Parser, Name) {
    assertParses<pdf::Name>("/SomeName", [](auto *result) {
        ASSERT_EQ(result->value(), "SomeName"); //
    });
}

TEST(Parser, ArrayEmpty) {
    assertParses<pdf::Array>("[]", [](auto *result) {
        ASSERT_EQ(result->values.size(), 0); //
    });
}

TEST(Parser, ArrayWithValues) {
    assertParses<pdf::Array>("[549 3.14 false /SomeName]", [](auto *result) {
        ASSERT_EQ(result->values.size(), 4); //
    });
}

TEST(Parser, ArrayNested) {
    assertParses<pdf::Array>("[549 [3.14 false] /SomeName]", [](auto *result) {
        ASSERT_EQ(result->values.size(), 3); //
    });
}

TEST(Parser, DictionaryEmpty) {
    assertParses<pdf::Dictionary>("<< >>", [](pdf::Dictionary *result) {
        ASSERT_EQ(result->values.size(), 0); //
    });
}

TEST(Parser, DictionaryWithValues) {
    assertParses<pdf::Dictionary>("<< /Type /Hello \n /Key /Value \n >>", [](pdf::Dictionary *result) {
        ASSERT_EQ(result->values.size(), 2); //
        ASSERT_EQ(result->values["Type"]->as<pdf::Name>()->value(), "Hello");
        ASSERT_EQ(result->values["Key"]->as<pdf::Name>()->value(), "Value");
    });
}

TEST(Parser, DictionaryNested) {
    assertParses<pdf::Dictionary>("<< /Type /Hello \n /Dict << /Key /Value \n >> \n >>", [](pdf::Dictionary *result) {
        ASSERT_EQ(result->values.size(), 2);
        ASSERT_EQ(result->values["Type"]->as<pdf::Name>()->value(), "Hello");
        ASSERT_EQ(result->values["Dict"]->as<pdf::Dictionary>()->values.size(), 1);
    });
}

TEST(Parser, IndirectReference) {
    assertParses<pdf::IndirectReference>("1 2 R", [](pdf::IndirectReference *result) {
        ASSERT_EQ(result->objectNumber, 1);
        ASSERT_EQ(result->generationNumber, 2);
    });
}

TEST(Parser, HexadecimalString) {
    assertParses<pdf::HexadecimalString>("<949FFBA879E60749D38B89A33E0DD9E7>", [](pdf::HexadecimalString *result) {
        ASSERT_EQ(result->data, "949FFBA879E60749D38B89A33E0DD9E7");
    });
    assertParses<pdf::HexadecimalString>("<48656c6c6f>", [](pdf::HexadecimalString *result) {
        ASSERT_EQ(result->data, "48656c6c6f");
        ASSERT_EQ(result->to_string(), "Hello");
    });
}

TEST(Parser, LiteralString) {
    assertParses<pdf::LiteralString>("(This is a string)", [](pdf::LiteralString *result) {
        ASSERT_EQ(result->value(), "This is a string"); //
    });
}

TEST(Parser, DictionaryTrailer) {
    const std::string input = "<</Size 9/Root 7 0 R\n"
                              "/Info 8 0 R\n"
                              "/ID [ <949FFBA879E60749D38B89A33E0DD9E7>\n"
                              "<949FFBA879E60749D38B89A33E0DD9E7> ]\n"
                              "/DocChecksum /87E47BA8C63C8BA796458FA05DBE8C32\n"
                              ">>";
    assertParses<pdf::Dictionary>(input, [](pdf::Dictionary *result) {
        ASSERT_EQ(result->values.size(), 5); //
        ASSERT_EQ(result->values["Size"]->as<pdf::Integer>()->value, 9);
        ASSERT_EQ(result->values["Info"]->as<pdf::IndirectReference>()->objectNumber, 8);
        ASSERT_EQ(result->values["Info"]->as<pdf::IndirectReference>()->generationNumber, 0);
        ASSERT_EQ(result->values["ID"]->as<pdf::Array>()->values.size(), 2);
        ASSERT_EQ(result->values["ID"]->as<pdf::Array>()->values[0]->as<pdf::HexadecimalString>()->data,
                  "949FFBA879E60749D38B89A33E0DD9E7");
        ASSERT_EQ(result->values["ID"]->as<pdf::Array>()->values[1]->as<pdf::HexadecimalString>()->data,
                  "949FFBA879E60749D38B89A33E0DD9E7");
        ASSERT_EQ(result->values["DocChecksum"]->as<pdf::Name>()->value(), "87E47BA8C63C8BA796458FA05DBE8C32");
    });
}

TEST(Parser, DictionaryStream) {
    assertParses<pdf::Dictionary>("<</Length 45/Filter/FlateDecode>>",
                                  [](pdf::Dictionary *result) { ASSERT_EQ(result->values.size(), 2); });
}

TEST(Parser, IndirectObjectWithEmptyDictionary) {
    assertParses<pdf::IndirectObject>("5 0 obj\n<<\n>>\nendobj", [](pdf::IndirectObject *result) {
        ASSERT_EQ(result->objectNumber, 5);
        ASSERT_EQ(result->generationNumber, 0);
        ASSERT_EQ(result->object->as<pdf::Dictionary>()->values.size(), 0);
    });
}

TEST(Parser, MoreThanOneObject) {
    assertParses<pdf::Array>("[/MyName] /AnotherName", [](pdf::Array *result) {
        ASSERT_EQ(result->values.size(), 1); //
    });
}

TEST(Parser, IndirectObject) {
    assertParses<pdf::IndirectObject>("12 1 obj \n /MyName \n endobj", [](pdf::IndirectObject *result) {
        ASSERT_EQ(result->objectNumber, 12);
        ASSERT_EQ(result->generationNumber, 1);
        ASSERT_EQ(result->object->as<pdf::Name>()->value(), "MyName");
    });
}

TEST(Parser, Stream) {
    const std::string input = "<</Length 10/Filter/FlateDecode>>\n"
                              "stream\n"
                              "some bytes\n"
                              "endstream";
    assertParses<pdf::Stream>(input, [&input](pdf::Stream *result) {
        ASSERT_EQ(result->data.length(), 61);
        ASSERT_EQ(result->data, input);
        ASSERT_EQ(result->streamData.length(), 10);
        ASSERT_EQ(result->streamData, "some bytes");
    });
}

TEST(Parser, StreamIndirectLength) {
    const std::string input                 = "<</Length 0 0 R/Filter/FlateDecode>>\n"
                                              "stream\n"
                                              "some bytes\n"
                                              "endstream";
    std::vector<pdf::IndirectObject *> refs = {new pdf::IndirectObject("3 0 R", 3, 0, new pdf::Integer("10", 10))};
    auto resolver                           = TestReferenceResolver(refs);
    assertParsesWithReferenceResolver<pdf::Stream>(input, &resolver, [&input](pdf::Stream *result) {
        ASSERT_EQ(result->data.length(), 64);
        ASSERT_EQ(result->data, input);
        ASSERT_EQ(result->streamData.length(), 10);
        ASSERT_EQ(result->streamData, "some bytes");
    });
    for (auto ref : refs) {
        delete ref;
    }
}

TEST(Parser, Null) {
    const std::string input = "null";
    assertParses<pdf::Null>(input, [](pdf::Null * /*result*/) {});
}

TEST(Parser, CatalogDict) {
    assertParses<pdf::IndirectObject>("7 0 obj\n"
                                      "<</Type/Catalog/Pages 4 0 R\n"
                                      "/OpenAction[1 0 R /XYZ null null 0]\n"
                                      "/Lang(en-US)\n"
                                      ">>\n"
                                      "endobj",
                                      [](pdf::IndirectObject * /*result*/) {});
}

TEST(Parser, MultipleObjects) {
    auto textProvider = pdf::StringTextProvider("5\n 6 7");
    auto lexer        = pdf::TextLexer(textProvider);
    auto allocator    = pdf::Allocator();
    allocator.init(1000);
    auto parser = pdf::Parser(lexer, allocator);

    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->type, pdf::Object::Type::INTEGER);
    ASSERT_EQ(result->as<pdf::Integer>()->value, 5);

    result = parser.parse();
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->type, pdf::Object::Type::INTEGER);
    ASSERT_EQ(result->as<pdf::Integer>()->value, 6);

    result = parser.parse();
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->type, pdf::Object::Type::INTEGER);
    ASSERT_EQ(result->as<pdf::Integer>()->value, 7);
}

TEST(Parser, IndirectObject1) {
    auto textProvider = pdf::StringTextProvider(
          "5 0 obj\n<<\n/Type /Metadata\n/Subtype /XML\n/Length 870>>\nstream\r\n<?xpacket begin='﻿' "
          "id='W5M0MpCehiHzreSzNTczkc9d'?>\r<x:xmpmeta xmlns:x=\"adobe:ns:meta/\">\r  <rdf:RDF "
          "xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\">\r    <rdf:Description "
          "xmlns:dc=\"http://purl.org/dc/elements/1.1/\" rdf:about=\"\">\r      <dc:creator>\r        <rdf:Seq>\r      "
          "    <rdf:li>veraPDF Consortium</rdf:li>\r        </rdf:Seq>\r      </dc:creator>\r    </rdf:Description>\r  "
          "  <rdf:Description xmlns:xmp=\"http://ns.adobe.com/xap/1.0/\" rdf:about=\"\" xmp:CreatorTool=\"veraPDF Test "
          "Builder\" xmp:CreateDate=\"2015-03-10T17:19:21+01:00\" xmp:ModifyDate=\"2015-03-10T17:19:21+01:00\"/>\r    "
          "<rdf:Description xmlns:pdf=\"http://ns.adobe.com/pdf/1.3/\" rdf:about=\"\" pdf:Producer=\"veraPDF Test "
          "Builder 1.0 \"/>\r    <rdf:Description xmlns:pdfaid=\"http://www.aiim.org/pdfa/ns/id/\" rdf:about=\"\" "
          "pdfaid:part=\"2\" pdfaid:conformance=\"B\"/>\r  </rdf:RDF>\r</x:xmpmeta>\r<?xpacket "
          "end='w'?>\rendstream\nendobj");
    auto lexer     = pdf::TextLexer(textProvider);
    auto allocator = pdf::Allocator();
    allocator.init(1000);
    auto parser = pdf::Parser(lexer, allocator);

    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->type, pdf::Object::Type::INDIRECT_OBJECT);
}
