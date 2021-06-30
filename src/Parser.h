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
        BOOLEAN    = 0,
        INTEGER    = 1,
        REAL       = 2,
        STRING     = 3,
        NAME       = 4,
        ARRAY      = 5,
        DICTIONARY = 6,
    };
    Type type;
    explicit Object(Type _type) : type(_type) {}
    template <typename T> T *as() {
        ASSERT(T::staticType() != type);
        return (T *)this;
    }
};

struct Boolean : public Object {
    bool value = false;
    explicit Boolean(bool b) : Object(staticType()), value(b) {}

    static Type staticType() { return Type::BOOLEAN; }
};

struct Integer : public Object {
    int64_t value = 0;
    explicit Integer(int64_t i) : Object(staticType()), value(i) {}
    static Type staticType() { return Type::INTEGER; }
};

struct Real : public Object {
    double value = 0;
    explicit Real(double d) : Object(staticType()), value(d) {}
    static Type staticType() { return Type::REAL; }
};

struct String : public Object {
    std::string value;
    String() : Object(staticType()) {}
    static Type staticType() { return Type::STRING; }
};

struct Name : public Object {
    std::string value;
    explicit Name(std::string string) : Object(staticType()), value(std::move(string)) {}
    static Type staticType() { return Type::NAME; }
};

struct Array : public Object {
    std::vector<Object *> values = {};
    explicit Array(std::vector<Object *> objects) : Object(staticType()), values(std::move(objects)) {}
    static Type staticType() { return Type::ARRAY; }
};

struct Dictionary : public Object {
    std::unordered_map<Name *, Object *> values = {};
    Dictionary() : Object(staticType()) {}
    static Type staticType() { return Type::DICTIONARY; }
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
    String *parseString();
    Name *parseName();
    Array *parseArray();
    Dictionary *parseDictionary();
};
