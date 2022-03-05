#pragma once

#include <cstdint>
#include <list>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "lexer.h"
#include "objects.h"
#include "util/allocator.h"
#include "util/util.h"

namespace pdf {

struct ReferenceResolver {
    virtual IndirectObject *resolve(const IndirectReference *reference) = 0;
};

struct NoopReferenceResolver : public ReferenceResolver {
    IndirectObject *resolve(const IndirectReference * /*reference*/) override { return nullptr; }
};

struct Parser {
    Lexer &lexer;
    util::Allocator &allocator;
    ReferenceResolver *referenceResolver;

    std::vector<Token> tokens = {};
    size_t currentTokenIdx    = 0;

    explicit Parser(Lexer &_lexer, util::Allocator &_allocator);
    explicit Parser(Lexer &_lexer, util::Allocator &_allocator, ReferenceResolver *_referenceResolver);

    Object *parse();

  private:
    void ignore_new_line_tokens();
    [[nodiscard]] bool ensure_tokens_have_been_lexed();
    [[nodiscard]] bool current_token_is(Token::Type type);

    Boolean *parse_boolean();
    Integer *parse_integer();
    Real *parse_real();
    Null *parse_null_object();
    LiteralString *parse_literal_string();
    HexadecimalString *parse_hexadecimal_string();
    Name *parse_name();
    Array *parse_array();
    Dictionary *parse_dictionary();
    IndirectReference *parse_indirect_reference();
    IndirectObject *parse_indirect_object();
    Object *parse_stream_or_dictionary();
};

} // namespace pdf
