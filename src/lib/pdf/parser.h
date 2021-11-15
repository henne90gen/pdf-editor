#pragma once

#include <cstdint>
#include <list>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "allocator.h"
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
    IndirectObject *resolve(const IndirectReference * /*reference*/) override { return nullptr; }
};

class Parser {
  public:
    explicit Parser(Lexer &_lexer, Allocator &_allocator);
    explicit Parser(Lexer &_lexer, Allocator &_allocator, ReferenceResolver *_referenceResolver);

    Object *parse();

  private:
    Lexer &lexer;
    Allocator &allocator;
    ReferenceResolver *referenceResolver;

    std::vector<Token> tokens = {};
    size_t currentTokenIdx    = 0;

    void ignoreNewLineTokens();
    [[nodiscard]] bool ensureTokensHaveBeenLexed();
    [[nodiscard]] bool currentTokenIs(Token::Type type);

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
