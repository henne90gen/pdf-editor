#include <gtest/gtest.h>
#include <sstream>
#include <vector>

#include <pdf/document.h>
#include <pdf/util/types.h>

namespace pdf {
void write_object(std::ostream &s, Object *object);
void write_null_object(std::ostream &s, Null *);
void write_boolean_object(std::ostream &s, Boolean *boolean);
void write_integer_object(std::ostream &s, Integer *integer);
void write_real_object(std::ostream &s, Real *real);
void write_hexadecimal_string_object(std::ostream &s, HexadecimalString *hexadecimal);
void write_literal_string_object(std::ostream &s, LiteralString *literal);
void write_name_object(std::ostream &s, Name *name);
void write_array_object(std::ostream &s, Array *array);
void write_dictionary_object(std::ostream &s, Dictionary *dictionary);
void write_indirect_reference_object(std::ostream &s, IndirectReference *reference);
void write_indirect_object(std::ostream &s, IndirectObject *object);
void write_stream_object(std::ostream &s, Stream *stream);
void write_object_stream_content_object(std::ostream &, ObjectStreamContent *);
} // namespace pdf

TEST(Writer, write_null) {
    std::stringstream s;
    pdf::write_null_object(s, new pdf::Null());
    const auto &str = s.str();
    ASSERT_EQ(str, "null");
}

TEST(Writer, write_boolean) {
    {
        std::stringstream s;
        pdf::write_boolean_object(s, new pdf::Boolean(true));
        const auto &str = s.str();
        ASSERT_EQ(str, "true");
    }
    {
        std::stringstream s;
        pdf::write_boolean_object(s, new pdf::Boolean(false));
        const auto &str = s.str();
        ASSERT_EQ(str, "false");
    }
}

TEST(Writer, write_integer) {
    std::stringstream s;
    pdf::write_integer_object(s, new pdf::Integer(123));
    const auto &str = s.str();
    ASSERT_EQ(str, "123");
}

TEST(Writer, write_real) {
    std::stringstream s;
    pdf::write_real_object(s, new pdf::Real(12.3));
    const auto &str = s.str();
    ASSERT_EQ(str, "12.300000");
}

TEST(Writer, write_hexadecimal_string) {
    std::stringstream s;
    pdf::write_hexadecimal_string_object(s, new pdf::HexadecimalString("abc123"));
    const auto &str = s.str();
    ASSERT_EQ(str, "<abc123>");
}

TEST(Writer, write_literal_string) {
    std::stringstream s;
    pdf::write_literal_string_object(s, new pdf::LiteralString("My String"));
    const auto &str = s.str();
    ASSERT_EQ(str, "(My String)");
}

TEST(Writer, write_name) {
    std::stringstream s;
    pdf::write_name_object(s, new pdf::Name("MyName"));
    const auto &str = s.str();
    ASSERT_EQ(str, "/MyName");
}

TEST(Writer, write_array) {
    auto allocator_result = pdf::Allocator::create();
    auto &allocator       = allocator_result.value();
    std::stringstream s;
    auto vec = pdf::Vector<pdf::Object *>(allocator);
    vec.push_back(new pdf::Name("Hello"));
    vec.push_back(new pdf::Name("World"));
    pdf::write_array_object(s, new pdf::Array(vec));
    const auto &str = s.str();
    ASSERT_EQ(str, "[/Hello /World]");
}

TEST(Writer, write_dictionary) {
    auto allocator_result = pdf::Allocator::create();
    auto &allocator       = allocator_result.value();
    std::stringstream s;
    auto map     = pdf::UnorderedMap<std::string, pdf::Object *>(allocator);
    map["Hello"] = new pdf::Integer(123);
    map["World"] = new pdf::LiteralString("World");
    pdf::write_dictionary_object(s, new pdf::Dictionary(map));
    const auto &str = s.str();
    ASSERT_EQ(str, "<</World (World) /Hello 123>>");
}

TEST(Writer, write_indirect_reference) {
    std::stringstream s;
    pdf::write_indirect_reference_object(s, new pdf::IndirectReference(12, 3));
    const auto &str = s.str();
    ASSERT_EQ(str, "12 3 R");
}

TEST(Writer, write_indirect_object) {
    std::stringstream s;
    pdf::write_indirect_object(s, new pdf::IndirectObject(12, 3, new pdf::Integer(5)));
    const auto &str = s.str();
    ASSERT_EQ(str, "12 3 obj\n5\nendobj\n");
}

TEST(Writer, write_stream) {
    auto allocator_result = pdf::Allocator::create();
    auto &allocator       = allocator_result.value();
    std::stringstream s;
    auto m    = pdf::UnorderedMap<std::string, pdf::Object *>(allocator);
    m["Size"] = new pdf::Integer(123);
    auto dict = new pdf::Dictionary(m);
    pdf::write_stream_object(s, new pdf::Stream(dict, "abc123"));
    const auto &str = s.str();
    ASSERT_EQ(str, "<</Size 123>>\nstream\nabc123\nendstream");
}
