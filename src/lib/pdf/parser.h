#pragma once

#include <cstdint>
#include <list>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "pdf/lexer.h"
#include "pdf/memory/arena_allocator.h"
#include "pdf/objects.h"
#include "pdf/util/debug.h"

namespace pdf {

struct ReferenceResolver {
    virtual IndirectObject *resolve(const IndirectReference *reference) = 0;
};

struct NoopReferenceResolver : public ReferenceResolver {
    IndirectObject *resolve(const IndirectReference * /*reference*/) override { return nullptr; }
};

struct Parser {
    Lexer &lexer;
    Arena &arena;
    ReferenceResolver *referenceResolver;

    Vector<Token> tokens;
    size_t currentTokenIdx = 0;

    explicit Parser(Lexer &_lexer, Arena &_arena);
    explicit Parser(Lexer &_lexer, Arena &_arena, ReferenceResolver *_referenceResolver);

    Object *parse();

    int64_t already_read_bytes();

  private:
    void ignore_new_lines_and_comments();
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
