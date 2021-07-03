#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "pdf_lexer.h"
#include "pdf_objects.h"
#include "util.h"

namespace pdf {

class Parser {
  public:
    explicit Parser(Lexer &_lexer) : lexer(_lexer) {}

    Object *parse();

  private:
    Lexer &lexer;
    std::vector<Token> tokens = {};
    int currentTokenIdx       = 0;
    [[nodiscard]] bool currentTokenIs(Token::Type type);

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
    IndirectObject *parseIndirectObject();
    Object *parseStreamOrDictionary();
};
} // namespace pdf
