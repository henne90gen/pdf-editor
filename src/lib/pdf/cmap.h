#pragma once

#include <utility>

#include "allocator.h"
#include "lexer.h"
#include "objects.h"

namespace pdf {

class CMap {
  public:
    explicit CMap(std::unordered_map<uint8_t, std::string> _charmap) : charmap(std::move(_charmap)) {}
    [[nodiscard]] std::optional<std::string> map_char_code(uint8_t code) {
        auto itr = charmap.find(code);
        if (itr == charmap.end()) {
            return {};
        }
        return itr->second;
    }

  private:
    std::unordered_map<uint8_t, std::string> charmap = {};
};

struct CMapParser {
    explicit CMapParser(Lexer &_lexer, Allocator &_allocator) : lexer(_lexer), allocator(_allocator) {}

    /// Attempts to parse a CMap from the given lexer
    CMap *parse(); // TODO maybe return by value instead of a pointer

  private:
    Lexer &lexer;
    Allocator &allocator;
    size_t currentTokenIdx    = 0;
    std::vector<Token> tokens = {};

    void ignoreNewLineTokens();
    [[nodiscard]] bool ensureTokensHaveBeenLexed();
    [[nodiscard]] bool currentTokenIs(Token::Type type);

    void parseCodeSpaceRange();
    void parseBfChar(std::unordered_map<uint8_t, std::string> &charmap);
    void parseBfRange(std::unordered_map<uint8_t, std::string> &charmap);
};

struct CMapStream : public Stream {
    std::optional<CMap *> read_cmap(Allocator &allocator);
};

} // namespace pdf
