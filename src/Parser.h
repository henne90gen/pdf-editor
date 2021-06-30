#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Lexer.h"

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
};

struct Boolean : public Object {
    bool value = false;
    explicit Boolean(bool b) : Object(Type::BOOLEAN), value(b) {}
};

struct Integer : public Object {
    int64_t value = 0;
    Integer() : Object(Type::INTEGER) {}
};

struct Real : public Object {
    double value = 0;
    Real() : Object(Type::REAL) {}
};

struct String : public Object {
    std::string value;
    String() : Object(Type::STRING) {}
};

struct Name : public Object {
    std::string value;
    Name() : Object(Type::NAME) {}
};

struct Array : public Object {
    std::vector<Object *> values = {};
    explicit Array(std::vector<Object *> objects) : Object(Type::ARRAY), values(std::move(objects)) {}
};

struct Dictionary : public Object {
    std::unordered_map<Name *, Object *> values = {};
    Dictionary() : Object(Type::DICTIONARY) {}
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
