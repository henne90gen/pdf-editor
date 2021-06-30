#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Lexer.h"
#include "util.h"

struct Object {
    enum class Type {
        BOOLEAN            = 0,
        INTEGER            = 1,
        REAL               = 2,
        HEXADECIMAL_STRING = 3,
        LITERAL_STRING     = 4,
        NAME               = 5,
        ARRAY              = 6,
        DICTIONARY         = 7,
        INDIRECT_REFERENCE = 8,
    };
    Type type;
    explicit Object(Type _type) : type(_type) {}
    template <typename T> T *as() {
        ASSERT(T::staticType() != type);
        return (T *)this;
    }
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

    std::string value;

    explicit LiteralString(std::string s) : Object(staticType()), value(std::move(s)) {}
};

struct HexadecimalString : public Object {
    static Type staticType() { return Type::HEXADECIMAL_STRING; }

    std::string value;

    explicit HexadecimalString(std::string s) : Object(staticType()), value(std::move(s)) {}
};

struct Name : public Object {
    static Type staticType() { return Type::NAME; }

    std::string value;

    explicit Name(std::string string) : Object(staticType()), value(std::move(string)) {}
};

struct Array : public Object {
    static Type staticType() { return Type::ARRAY; }

    std::vector<Object *> values = {};

    explicit Array(std::vector<Object *> objects) : Object(staticType()), values(std::move(objects)) {}
};

struct Dictionary : public Object {
    static Type staticType() { return Type::DICTIONARY; }

    std::unordered_map<Name *, Object *> values = {};

    explicit Dictionary(std::unordered_map<Name *, Object *> map) : Object(staticType()), values(std::move(map)) {}
};

struct IndirectReference : public Object {
    static Type staticType() { return Type::INDIRECT_REFERENCE; }

    int64_t typeId         = 0;
    int64_t revisionNumber = 0;

    explicit IndirectReference(int64_t _typeId, int64_t _revisionNumber)
        : Object(staticType()), typeId(_typeId), revisionNumber(_revisionNumber) {}
};

class Parser {
  public:
    explicit Parser(Lexer &_lexer) : lexer(_lexer) {}

    Object *parse();

  private:
    Lexer &lexer;
    std::vector<Token> tokens = {};
    int currentTokenIdx       = 0;
    [[nodiscard]] bool currentTokenIs(Token::Type type) const;

    Object *parseObject();
    Boolean *parseBoolean();
    Integer *parseInteger();
    Real *parseReal();
    LiteralString *parseLiteralString();
    HexadecimalString *parseHexadecimalString();
    Name *parseName();
    Array *parseArray();
    Dictionary *parseDictionary();
    IndirectReference *parseIndirectReference();
};
