#pragma once

#include <iostream>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

#include "pdf/memory/arena_allocator.h"
#include "pdf/util/debug.h"
#include "pdf/util/types.h"

namespace pdf {

struct Document;

#define ENUMERATE_OBJECT_TYPES(O)                                                                                      \
    O(OBJECT)                                                                                                          \
    O(BOOLEAN)                                                                                                         \
    O(INTEGER)                                                                                                         \
    O(REAL)                                                                                                            \
    O(HEXADECIMAL_STRING)                                                                                              \
    O(LITERAL_STRING)                                                                                                  \
    O(NAME)                                                                                                            \
    O(ARRAY)                                                                                                           \
    O(DICTIONARY)                                                                                                      \
    O(INDIRECT_REFERENCE)                                                                                              \
    O(INDIRECT_OBJECT)                                                                                                 \
    O(STREAM)                                                                                                          \
    O(NULL_OBJECT)                                                                                                     \
    O(OBJECT_STREAM_CONTENT)

struct Object {
    enum class Type {
#define DECLARE_ENUM(Name) Name,
        ENUMERATE_OBJECT_TYPES(DECLARE_ENUM)
#undef DECLARE_ENUM
    };

    Type type;

    explicit Object(Type _type) : type(_type) {}

    static Type staticType() { return Type::OBJECT; }
    template <typename T> T *as() {
        ASSERT(T::staticType() == type || T::staticType() == Type::OBJECT);
        return (T *)this;
    }
    template <typename T> bool is() { return T::staticType() == type || T::staticType() == Type::OBJECT; }
    std::string type_string() const;
};

struct Boolean : public Object {
    bool value = false;

    static Type staticType() { return Type::BOOLEAN; }
    explicit Boolean(bool b) : Object(staticType()), value(b) {}
};

struct Integer : public Object {
    int64_t value = 0;

    static Type staticType() { return Type::INTEGER; }
    explicit Integer(int64_t i) : Object(staticType()), value(i) {}

    void set(Document &document, int64_t i);
};

struct Real : public Object {
    double value = 0;

    static Type staticType() { return Type::REAL; }
    explicit Real(double d) : Object(staticType()), value(d) {}
};

struct LiteralString : public Object {
    std::string value;

    static Type staticType() { return Type::LITERAL_STRING; }
    explicit LiteralString(std::string _value) : Object(staticType()), value(std::move(_value)) {}
};

struct HexadecimalString : public Object {
    std::string value;

    static Type staticType() { return Type::HEXADECIMAL_STRING; }
    explicit HexadecimalString(std::string _value) : Object(staticType()), value(std::move(_value)) {}

    /// decodes the hexadecimal string
    [[nodiscard]] std::string to_string() const;
};

struct Name : public Object {
    std::string value;

    static Type staticType() { return Type::NAME; }
    explicit Name(std::string _value) : Object(staticType()), value(std::move(_value)) {}
};

struct Array : public Object {
    Vector<Object *> values;

    static Type staticType() { return Type::ARRAY; }
    explicit Array(Vector<Object *> objects) : Object(staticType()), values(std::move(objects)) {}

    void remove_element(Document &document, size_t index);
};

struct Dictionary : public Object {
    UnorderedMap<std::string, Object *> values;

    static Type staticType() { return Type::DICTIONARY; }
    explicit Dictionary(UnorderedMap<std::string, Object *> map) : Object(staticType()), values(std::move(map)) {}

    template <typename T> std::optional<T *> find(const std::string &key) {
        auto itr = values.find(key);
        if (itr == values.end()) {
            return {};
        }
        return itr->second->as<T>();
    }

    template <typename T> T *must_find(const std::string &key) {
        auto itr = values.find(key);
        if (itr == values.end()) {
            return nullptr;
        }
        return itr->second->as<T>();
    }
};

struct IndirectReference : public Object {
    int64_t objectNumber     = 0;
    int64_t generationNumber = 0;

    static Type staticType() { return Type::INDIRECT_REFERENCE; }
    explicit IndirectReference(int64_t _objectNumber, int64_t _generationNumber)
        : Object(staticType()), objectNumber(_objectNumber), generationNumber(_generationNumber) {}
};

struct IndirectObject : public Object {
    int64_t objectNumber     = 0;
    int64_t generationNumber = 0;
    Object *object           = nullptr;

    static Type staticType() { return Type::INDIRECT_OBJECT; }
    explicit IndirectObject(int64_t _objectNumber, int64_t _generationNumber, Object *_object)
        : Object(staticType()), objectNumber(_objectNumber), generationNumber(_generationNumber), object(_object) {}
};

struct Stream : public Object {
    Dictionary *dictionary = nullptr;
    std::string_view streamData;

    const uint8_t *decodedStream = nullptr;
    size_t decodedStreamSize     = 0;

    static Type staticType() { return Type::STREAM; }
    explicit Stream(Dictionary *_dictionary, std::string_view encodedData)
        : Object(staticType()), dictionary(_dictionary), streamData(encodedData) {}

    static Stream *create_from_unencoded_data(Allocator &allocator,
                                              const UnorderedMap<std::string, Object *> &additionalDictionaryEntries,
                                              std::string_view unencodedData);

    [[nodiscard]] std::string_view decode(Allocator &allocator);
    void encode(Allocator &allocator, const std::string &data);
    [[nodiscard]] std::vector<std::string> filters() const;
};

struct EmbeddedFile : public Stream {
    std::optional<int64_t> size();
    // TODO std::optional creation_date();
    // TODO std::optional mod_date();
    // TODO std::optional check_sum();
};

struct Null : public Object {
    static Type staticType() { return Type::NULL_OBJECT; }
    explicit Null() : Object(staticType()) {}
};

struct ObjectStreamContent : public Object {
    static Type staticType() { return Type::OBJECT_STREAM_CONTENT; }
    explicit ObjectStreamContent() : Object(staticType()) {}
};

std::ostream &operator<<(std::ostream &os, Object::Type &type);

} // namespace pdf
