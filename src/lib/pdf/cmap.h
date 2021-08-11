#pragma once

#include "objects.h"
#include "lexer.h"

namespace pdf {

struct CMap {
    explicit CMap(const std::string_view _data) : data(_data) {}

    std::string_view data;
};

struct CMapParser {
    explicit CMapParser(Lexer &_lexer) : lexer(_lexer) {}

    CMap *parse();

  private:
    bool currentTokenIs(Token::Type type);

    Lexer &lexer;
    int currentTokenIdx       = 0;
    std::vector<Token> tokens = {};
};

struct CMapStream : public Stream {
    std::optional<CMap *> read_cmap();
};

} // namespace pdf
