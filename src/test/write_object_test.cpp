#include <gtest/gtest.h>
#include <sstream>
#include <vector>

#include <pdf/document.h>
#include <pdf/helper/static_vector.h>

namespace pdf {
size_t write_object(std::ostream &s, Object *object);
size_t write_null_object(std::ostream &s, Null *);
size_t write_boolean_object(std::ostream &s, Boolean *boolean);
size_t write_integer_object(std::ostream &s, Integer *integer);
size_t write_real_object(std::ostream &s, Real *real);
size_t write_hexadecimal_string_object(std::ostream &s, HexadecimalString *hexadecimal);
size_t write_literal_string_object(std::ostream &s, LiteralString *literal);
size_t write_name_object(std::ostream &s, Name *name);
size_t write_array_object(std::ostream &s, Array *array);
size_t write_dictionary_object(std::ostream &s, Dictionary *dictionary);
size_t write_indirect_reference_object(std::ostream &s, IndirectReference *reference);
size_t write_indirect_object(std::ostream &s, IndirectObject *object);
size_t write_stream_object(std::ostream &s, Stream *stream);
size_t write_object_stream_content_object(std::ostream &, ObjectStreamContent *);
} // namespace pdf

TEST(Writer, write_null) {
    std::stringstream s;
    auto bytesWritten = pdf::write_null_object(s, new pdf::Null());
    const auto &str   = s.str();
    ASSERT_EQ(str.size(), bytesWritten);
    ASSERT_EQ(str, "null");
}

TEST(Writer, write_boolean) {
    {
        std::stringstream s;
        auto bytesWritten = pdf::write_boolean_object(s, new pdf::Boolean(true));
        const auto &str   = s.str();
        ASSERT_EQ(str.size(), bytesWritten);
        ASSERT_EQ(str, "true");
    }
    {
        std::stringstream s;
        auto bytesWritten = pdf::write_boolean_object(s, new pdf::Boolean(false));
        const auto &str   = s.str();
        ASSERT_EQ(str.size(), bytesWritten);
        ASSERT_EQ(str, "false");
    }
}

TEST(Writer, write_integer) {
    std::stringstream s;
    auto bytesWritten = pdf::write_integer_object(s, new pdf::Integer(123));
    const auto &str   = s.str();
    ASSERT_EQ(str.size(), bytesWritten);
    ASSERT_EQ(str, "123");
}

TEST(Writer, write_real) {
    std::stringstream s;
    auto bytesWritten = pdf::write_real_object(s, new pdf::Real(12.3));
    const auto &str   = s.str();
    ASSERT_EQ(str.size(), bytesWritten);
    ASSERT_EQ(str, "12.300000");
}

TEST(Writer, write_hexadecimal_string) {
    std::stringstream s;
    auto bytesWritten = pdf::write_hexadecimal_string_object(s, new pdf::HexadecimalString("abc123"));
    const auto &str   = s.str();
    ASSERT_EQ(str.size(), bytesWritten);
    ASSERT_EQ(str, "<abc123>");
}

TEST(Writer, write_literal_string) {
    std::stringstream s;
    auto bytesWritten = pdf::write_literal_string_object(s, new pdf::LiteralString("My String"));
    const auto &str   = s.str();
    ASSERT_EQ(str.size(), bytesWritten);
    ASSERT_EQ(str, "(My String)");
}

TEST(Writer, write_name) {
    std::stringstream s;
    auto bytesWritten = pdf::write_name_object(s, new pdf::Name("MyName"));
    const auto &str   = s.str();
    ASSERT_EQ(str.size(), bytesWritten);
    ASSERT_EQ(str, "/MyName");
}

TEST(Writer, write_array) {
    std::stringstream s;
    pdf::Allocator allocator = {};
    allocator.init(100);
    auto vec = std::vector<pdf::Object *>();
    vec.push_back(new pdf::Name("Hello"));
    vec.push_back(new pdf::Name("World"));
    const auto &staticVector = pdf::StaticVector<pdf::Object *>::create(allocator, vec);
    const auto bytesWritten  = pdf::write_array_object(s, new pdf::Array(staticVector));
    const auto &str          = s.str();
    ASSERT_EQ(str.size(), bytesWritten);
    ASSERT_EQ(str, "[/Hello /World]");
}

TEST(Writer, write_dictionary) {
    std::stringstream s;
    pdf::Allocator allocator = {};
    allocator.init(100);
    auto map                = std::unordered_map<std::string, pdf::Object *>();
    map["Hello"]            = new pdf::Integer(123);
    map["World"]            = new pdf::LiteralString("World");
    const auto &staticMap   = pdf::StaticMap<std::string, pdf::Object *>::create(allocator, map);
    const auto bytesWritten = pdf::write_dictionary_object(s, new pdf::Dictionary(staticMap));
    const auto &str         = s.view();
    ASSERT_EQ(str.size(), bytesWritten);
    ASSERT_EQ(str, "<</World (World) /Hello 123>>");
}

TEST(Writer, write_indirect_reference) {
    std::stringstream s;
    auto bytesWritten = pdf::write_indirect_reference_object(s, new pdf::IndirectReference(12, 3));
    const auto &str   = s.str();
    ASSERT_EQ(str.size(), bytesWritten);
    ASSERT_EQ(str, "12 3 R");
}

TEST(Writer, write_indirect_object) {
    std::stringstream s;
    auto bytesWritten = pdf::write_indirect_object(s, new pdf::IndirectObject(12, 3, new pdf::Integer(5)));
    const auto &str   = s.str();
    ASSERT_EQ(str.size(), bytesWritten);
    ASSERT_EQ(str, "12 3 obj\n5\nendobj");
}

TEST(Writer, write_stream) {
    std::stringstream s;
    pdf::Allocator allocator = {};
    allocator.init(100);
    auto m            = std::unordered_map<std::string, pdf::Object *>();
    m["Size"]         = new pdf::Integer( 123);
    auto map          = pdf::StaticMap<std::string, pdf::Object *>::create(allocator, m);
    auto dict         = new pdf::Dictionary( map);
    auto bytesWritten = pdf::write_stream_object(s, new pdf::Stream( dict, "abc123"));
    const auto &str   = s.str();
    ASSERT_EQ(str.size(), bytesWritten);
    ASSERT_EQ(str, "<</Size 123>>\nstream\nabc123\nendstream");
}
