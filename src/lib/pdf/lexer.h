#pragma once

#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace pdf {

struct Token {
    enum class Type {
        INVALID,
        NEW_LINE,
        BOOLEAN,
        INTEGER,
        REAL,
        LITERAL_STRING,
        HEXADECIMAL_STRING,
        NAME,
        ARRAY_START,
        ARRAY_END,
        DICTIONARY_START,
        DICTIONARY_END,
        INDIRECT_REFERENCE,
        OBJECT_START,
        OBJECT_END,
        STREAM_START,
        STREAM_END,
        NULL_OBJ,
        OPERATOR,
        COMMENT, // TODO find out whether this is part of the pdf standard

        BEGIN,
        END,
        FIND_RESOURCE,
        DEFINE_RESOURCE,
        DEF,
        DICT,
        DUP,
        POP,
        CMAP_NAME,
        CURRENT_DICT,

        CMAP_BEGIN,
        CMAP_END,
        CMAP_USE,
        CMAP_BEGIN_CODE_SPACE_RANGE,
        CMAP_END_CODE_SPACE_RANGE,
        CMAP_USE_FONT,
        CMAP_BEGIN_BF_CHAR,
        CMAP_END_BF_CHAR,
        CMAP_BEGIN_BF_RANGE,
        CMAP_END_BF_RANGE,
        CMAP_BEGIN_CID_CHAR,
        CMAP_END_CID_CHAR,
        CMAP_BEGIN_CID_RANGE,
        CMAP_END_CID_RANGE,
        CMAP_BEGIN_NOTDEF_CHAR,
        CMAP_END_NOTDEF_CHAR,
        CMAP_BEGIN_NOTDEF_RANGE,
        CMAP_END_NOTDEF_RANGE,
    };

    Type type;
    std::string_view content;

    Token(Type _type, std::string_view _content) : type(_type), content(_content) {}
};

struct TextProvider {
    virtual std::optional<std::string_view> get_text() = 0;
};

struct StringTextProvider : public TextProvider {
    std::string_view text;
    bool hasBeenQueried = false;

    explicit StringTextProvider(const char *_text) : text(_text) {}
    explicit StringTextProvider(const std::string &_text) : text(_text) {}
    explicit StringTextProvider(const std::string_view &_text) : text(_text) {}

    std::optional<std::string_view> get_text() override {
        if (hasBeenQueried) {
            return {};
        }
        hasBeenQueried = true;
        return text;
    }
};

struct Lexer {
    virtual std::optional<Token> get_token()                   = 0;
    virtual std::string_view advance_stream(size_t characters) = 0;
};

struct TextLexer : public Lexer {
    TextProvider &textProvider;
    std::string_view currentWord;

    explicit TextLexer(TextProvider &_textProvider) : textProvider(_textProvider) {}

    std::optional<Token> get_token() override;
    std::string_view advance_stream(size_t characters) override;
};

struct TokenLexer : public Lexer {
    const std::vector<Token> &tokens;
    size_t currentTokenIdx = 0;

    explicit TokenLexer(const std::vector<Token> &_tokens) : tokens(_tokens) {}

    std::optional<Token> get_token() override {
        if (tokens.empty() || currentTokenIdx >= tokens.size()) {
            return {};
        }
        return tokens[currentTokenIdx++];
    }

    std::string_view advance_stream(size_t /*characters*/) override { return ""; }
};

} // namespace pdf
