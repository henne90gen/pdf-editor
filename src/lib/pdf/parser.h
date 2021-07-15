#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "lexer.h"
#include "objects.h"
#include "util.h"

namespace pdf {

class ReferenceResolver {
  public:
    virtual IndirectObject *resolve(const IndirectReference *reference) = 0;
};

class NoopReferenceResolver : public ReferenceResolver {
  public:
    IndirectObject *resolve(const IndirectReference *reference) override { return nullptr; }
};

class Parser {
  public:
    explicit Parser(Lexer &_lexer) : lexer(_lexer), referenceResolver(new NoopReferenceResolver()) {}

    explicit Parser(Lexer &_lexer, ReferenceResolver *_referenceResolver)
        : lexer(_lexer), referenceResolver(_referenceResolver) {}

    Object *parse();

  private:
    Lexer &lexer;
    ReferenceResolver *referenceResolver;
    std::vector<Token> tokens = {};
    int currentTokenIdx       = 0;
    [[nodiscard]] bool currentTokenIs(Token::Type type);

    Object *parseObject();
    Boolean *parseBoolean();
    Integer *parseInteger();
    Real *parseReal();
    Null *parseNullObject();
    LiteralString *parseLiteralString();
    HexadecimalString *parseHexadecimalString();
    Name *parseName();
    Array *parseArray();
    Dictionary *parseDictionary();
    IndirectReference *parseIndirectReference();
    IndirectObject *parseIndirectObject();
    Object *parseStreamOrDictionary();
};
} // namespace pdf
