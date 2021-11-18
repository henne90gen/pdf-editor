#pragma once

#include <iostream>
#include <optional>
#include <unordered_map>
#include <vector>

#include "helper/allocator.h"
#include "helper/static_map.h"
#include "helper/static_vector.h"
#include "helper/util.h"

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
    std::string_view data;

    explicit Object(Type _type, std::string_view _data) : type(_type), data(_data) {}
    virtual ~Object() = default;

    static Type staticType() { return Type::OBJECT; }
    template <typename T> T *as() {
        ASSERT(T::staticType() == type || T::staticType() == Type::OBJECT);
        return (T *)this;
    }
    template <typename T> bool is() { return T::staticType() == type || T::staticType() == Type::OBJECT; }
};

struct Boolean : public Object {
    bool value = false;

    static Type staticType() { return Type::BOOLEAN; }
    explicit Boolean(std::string_view data, bool b) : Object(staticType(), data), value(b) {}
};

struct Integer : public Object {
    int64_t value = 0;

    static Type staticType() { return Type::INTEGER; }
    explicit Integer(std::string_view data, int64_t i) : Object(staticType(), data), value(i) {}

    void set(Document &document, int64_t i);
};

struct Real : public Object {
    double value = 0;

    static Type staticType() { return Type::REAL; }
    explicit Real(std::string_view data, double d) : Object(staticType(), data), value(d) {}
};

struct LiteralString : public Object {
    static Type staticType() { return Type::LITERAL_STRING; }
    explicit LiteralString(std::string_view data) : Object(staticType(), data) {}

    [[nodiscard]] std::string_view value() const { return data.substr(1, data.size() - 2); }
};

struct HexadecimalString : public Object {
    static Type staticType() { return Type::HEXADECIMAL_STRING; }
    explicit HexadecimalString(std::string_view _data) : Object(staticType(), _data) {
        if (data[0] == '<') {
            data = data.substr(1, data.size() - 2);
        }
    }

    [[nodiscard]] std::string to_string() const;
};

struct Name : public Object {
    static Type staticType() { return Type::NAME; }
    explicit Name(std::string_view data) : Object(staticType(), data) {}

    [[nodiscard]] std::string_view value() { return data; }
};

struct Array : public Object {
    StaticVector<Object *> values = {};

    static Type staticType() { return Type::ARRAY; }
    explicit Array(std::string_view data, StaticVector<Object *> objects)
        : Object(staticType(), data), values(objects) {}

    void remove_element(Document &document, size_t index);
};

struct Dictionary : public Object {
    StaticMap<std::string, Object *> values = {};

    static Type staticType() { return Type::DICTIONARY; }
    explicit Dictionary(std::string_view data, StaticMap<std::string, Object *> map)
        : Object(staticType(), data), values(map) {}

    template <typename T> std::optional<T *> find(const std::string &key) {
        auto opt = values.find(key);
        if (!opt.has_value()) {
            return {};
        }
        return opt.value()->as<T>();
    }

    template <typename T> T *must_find(const std::string &key) {
        auto opt = values.find(key);
        if (!opt.has_value()) {
            return nullptr;
        }
        return opt.value()->as<T>();
    }
};

struct IndirectReference : public Object {
    int64_t objectNumber     = 0;
    int64_t generationNumber = 0;

    static Type staticType() { return Type::INDIRECT_REFERENCE; }
    explicit IndirectReference(std::string_view data, int64_t _objectNumber, int64_t _generationNumber)
        : Object(staticType(), data), objectNumber(_objectNumber), generationNumber(_generationNumber) {}
};

struct IndirectObject : public Object {
    int64_t objectNumber     = 0;
    int64_t generationNumber = 0;
    Object *object           = nullptr;

    static Type staticType() { return Type::INDIRECT_OBJECT; }
    explicit IndirectObject(std::string_view data, int64_t _objectNumber, int64_t _generationNumber, Object *_object)
        : Object(staticType(), data), objectNumber(_objectNumber), generationNumber(_generationNumber),
          object(_object) {}
};

struct Stream : public Object {
    Dictionary *dictionary = nullptr;
    std::string_view streamData;
    const char *decodedStream = nullptr;
    size_t decodedStreamSize  = 0;

    static Type staticType() { return Type::STREAM; }
    explicit Stream(std::string_view data, Dictionary *_dictionary, std::string_view _stream_data)
        : Object(staticType(), data), dictionary(_dictionary), streamData(_stream_data) {}

    [[nodiscard]] std::string_view decode(Allocator &allocator);
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

std::string to_string(Object *object);

} // namespace pdf
