#pragma once

#include <iostream>
#include <optional>
#include <unordered_map>
#include <vector>

#include "util.h"

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
    explicit Object(Type _type, std::string_view _data) : type(_type), data(_data) {}
    virtual ~Object() = default;

    static Type staticType() { return Type::OBJECT; }
    template <typename T> T *as() {
        ASSERT(T::staticType() == type || T::staticType() == Type::OBJECT);
        return (T *)this;
    }
    template <typename T> bool is() { return T::staticType() == type || T::staticType() == Type::OBJECT; }

    Type type;
    std::string_view data;
};

struct Boolean : public Object {
    static Type staticType() { return Type::BOOLEAN; }

    bool value = false;

    explicit Boolean(std::string_view data, bool b) : Object(staticType(), data), value(b) {}
};

struct Integer : public Object {
    static Type staticType() { return Type::INTEGER; }

    int64_t value = 0;

    explicit Integer(std::string_view data, int64_t i) : Object(staticType(), data), value(i) {}
};

struct Real : public Object {
    static Type staticType() { return Type::REAL; }

    double value = 0;

    explicit Real(std::string_view data, double d) : Object(staticType(), data), value(d) {}
};

struct LiteralString : public Object {
    static Type staticType() { return Type::LITERAL_STRING; }

    explicit LiteralString(std::string_view data) : Object(staticType(), data) {}
    std::string_view value() { return data.substr(1, data.size() - 2); }
};

struct HexadecimalString : public Object {
    static Type staticType() { return Type::HEXADECIMAL_STRING; }

    explicit HexadecimalString(std::string_view data) : Object(staticType(), data) {}

    [[nodiscard]] std::string to_string() const;
};

struct Name : public Object {
    static Type staticType() { return Type::NAME; }

    explicit Name(std::string_view data) : Object(staticType(), data) {}
    [[nodiscard]] std::string_view value() { return data; }
};

struct Array : public Object {
    static Type staticType() { return Type::ARRAY; }

    std::vector<Object *> values = {};

    explicit Array(std::string_view data, std::vector<Object *> objects)
        : Object(staticType(), data), values(std::move(objects)) {}

    void remove_element(Document &document, size_t index);
};

struct Dictionary : public Object {
    static Type staticType() { return Type::DICTIONARY; }

    std::unordered_map<std::string, Object *> values = {};

    explicit Dictionary(std::string_view data, std::unordered_map<std::string, Object *> map)
        : Object(staticType(), data), values(std::move(map)) {}

    template <typename T> std::optional<T *> find(const std::string &key) {
        auto itr = values.find(key);
        if (itr == values.end()) {
            return {};
        }
        return itr->second->as<T>();
    }
};

struct IndirectReference : public Object {
    static Type staticType() { return Type::INDIRECT_REFERENCE; }

    int64_t objectNumber     = 0;
    int64_t generationNumber = 0;

    explicit IndirectReference(std::string_view data, int64_t _objectNumber, int64_t _generationNumber)
        : Object(staticType(), data), objectNumber(_objectNumber), generationNumber(_generationNumber) {}
};

struct IndirectObject : public Object {
    static Type staticType() { return Type::INDIRECT_OBJECT; }

    int64_t objectNumber     = 0;
    int64_t generationNumber = 0;
    Object *object           = nullptr;

    explicit IndirectObject(std::string_view data, int64_t _objectNumber, int64_t _generationNumber, Object *_object)
        : Object(staticType(), data), objectNumber(_objectNumber), generationNumber(_generationNumber),
          object(_object) {}
};

class OperatorParser;

struct Stream : public Object {
    static Type staticType() { return Type::STREAM; }

    Dictionary *dictionary = nullptr;
    std::string_view stream_data;

    explicit Stream(std::string_view data, Dictionary *_dictionary, std::string_view _stream_data)
        : Object(staticType(), data), dictionary(_dictionary), stream_data(_stream_data) {}

    [[nodiscard]] std::string_view to_string() const;
    [[nodiscard]] std::vector<std::string> filters() const;
};

struct Null : public Object {
    static Type staticType() { return Type::NULL_OBJECT; }

    explicit Null(std::string_view data) : Object(staticType(), data) {}
};

struct ObjectStreamContent : public Object {
    static Type staticType() { return Type::OBJECT_STREAM_CONTENT; }

    explicit ObjectStreamContent(std::string_view data) : Object(staticType(), data) {}
};

std::ostream &operator<<(std::ostream &os, Object::Type &type);

} // namespace pdf
