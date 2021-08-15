#pragma once

#include <iostream>
#include <optional>
#include <unordered_map>
#include <vector>

#include "util.h"

namespace pdf {

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
    virtual ~Object() = default;

    static Type staticType() { return Type::OBJECT; }
    template <typename T> T *as() {
        ASSERT(T::staticType() == type || T::staticType() == Type::OBJECT);
        return (T *)this;
    }
    template <typename T> bool is() { return T::staticType() == type; }
};

struct Boolean : public Object {
    static Type staticType() { return Type::BOOLEAN; }

    bool value = false;

    explicit Boolean(bool b) : Object(staticType()), value(b) {}
};

struct Integer : public Object {
    static Type staticType() { return Type::INTEGER; }

    int64_t value = 0;

    explicit Integer(int64_t i) : Object(staticType()), value(i) {}
};

struct Real : public Object {
    static Type staticType() { return Type::REAL; }

    double value = 0;

    explicit Real(double d) : Object(staticType()), value(d) {}
};

struct LiteralString : public Object {
    static Type staticType() { return Type::LITERAL_STRING; }

    std::string_view value;

    explicit LiteralString(std::string_view s) : Object(staticType()), value(s) {}
};

struct HexadecimalString : public Object {
    static Type staticType() { return Type::HEXADECIMAL_STRING; }

    std::string_view value;

    explicit HexadecimalString(std::string_view s) : Object(staticType()), value(s) {}

    [[nodiscard]] std::string to_string() const;
};

struct Name : public Object {
    static Type staticType() { return Type::NAME; }

    std::string_view value;

    explicit Name(std::string_view s) : Object(staticType()), value(s) {}
};

struct Array : public Object {
    static Type staticType() { return Type::ARRAY; }

    std::vector<Object *> values = {};

    explicit Array(std::vector<Object *> objects) : Object(staticType()), values(std::move(objects)) {}
};

struct Dictionary : public Object {
    static Type staticType() { return Type::DICTIONARY; }

    std::unordered_map<std::string, Object *> values = {};

    explicit Dictionary(std::unordered_map<std::string, Object *> map) : Object(staticType()), values(std::move(map)) {}

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

    explicit IndirectReference(int64_t _objectNumber, int64_t _generationNumber)
        : Object(staticType()), objectNumber(_objectNumber), generationNumber(_generationNumber) {}
};

struct IndirectObject : public Object {
    static Type staticType() { return Type::INDIRECT_OBJECT; }

    int64_t objectNumber     = 0;
    int64_t generationNumber = 0;
    Object *object           = nullptr;

    explicit IndirectObject(int64_t _objectNumber, int64_t _generationNumber, Object *_object)
        : Object(staticType()), objectNumber(_objectNumber), generationNumber(_generationNumber), object(_object) {}
};

class OperatorParser;

struct Stream : public Object {
    static Type staticType() { return Type::STREAM; }

    Dictionary *dictionary = nullptr;
    std::string_view data;

    explicit Stream(Dictionary *_dictionary, std::string_view _data)
        : Object(staticType()), dictionary(_dictionary), data(_data) {}

    [[nodiscard]] std::string_view to_string() const;
    [[nodiscard]] std::vector<std::string> filters() const;
};

struct Null : public Object {
    static Type staticType() { return Type::NULL_OBJECT; }

    explicit Null() : Object(staticType()) {}
};

struct ObjectStreamContent : public Object {
    static Type staticType() { return Type::OBJECT_STREAM_CONTENT; }
};

std::ostream &operator<<(std::ostream &os, Object::Type &type);

} // namespace pdf
