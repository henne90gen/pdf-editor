#pragma once

#include <utility>

#include "pdf/lexer.h"
#include "pdf/memory/arena_allocator.h"
#include "pdf/objects.h"

namespace pdf {

struct CMap {
    std::unordered_map<uint8_t, std::string> charmap = {};

    explicit CMap(std::unordered_map<uint8_t, std::string> _charmap) : charmap(std::move(_charmap)) {}

    [[nodiscard]] std::optional<std::string> map_char_code(uint8_t code) const;
    [[nodiscard]] std::string map_char_codes(HexadecimalString *str) const;
};

struct CMapParser {
    Lexer &lexer;
    Allocator &allocator;
    size_t currentTokenIdx    = 0;
    std::vector<Token> tokens = {};

    explicit CMapParser(Lexer &_lexer, Allocator &_allocator) : lexer(_lexer), allocator(_allocator) {}

    /// Attempts to parse a CMap from the given lexer
    CMap *parse(); // TODO maybe return by value instead of a pointer

  private:
    void ignore_new_line_tokens();
    [[nodiscard]] bool ensure_tokens_have_been_lexed();
    [[nodiscard]] bool current_token_is(Token::Type type);

    void parse_code_space_range();
    void parse_bf_char(std::unordered_map<uint8_t, std::string> &charmap);
    void parse_bf_range(std::unordered_map<uint8_t, std::string> &charmap);
};

struct CMapStream : public Stream {
    std::optional<CMap *> read_cmap(Allocator &allocator);
};

} // namespace pdf
