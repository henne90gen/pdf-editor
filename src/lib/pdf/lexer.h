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

class TextProvider {
  public:
    virtual std::optional<std::string_view> getText() = 0;
};

class StringTextProvider : public TextProvider {
  public:
    explicit StringTextProvider(const char *_text) : text(_text) {}
    explicit StringTextProvider(const std::string &_text) : text(_text) {}
    explicit StringTextProvider(const std::string_view &_text) : text(_text) {}

    std::optional<std::string_view> getText() override {
        if (hasBeenQueried) {
            return {};
        }
        hasBeenQueried = true;
        return std::optional(text);
    }

  private:
    std::string_view text;
    bool hasBeenQueried = false;
};

class Lexer {
  public:
    virtual std::optional<Token> getToken()                   = 0;
    virtual std::string_view advanceStream(size_t characters) = 0;
};

class TextLexer : public Lexer {
  public:
    explicit TextLexer(TextProvider &_textProvider) : textProvider(_textProvider) {}

    std::optional<Token> getToken() override;
    std::string_view advanceStream(size_t characters) override;

  private:
    TextProvider &textProvider;
    std::string_view currentWord;
};

class TokenLexer : public Lexer {
  public:
    explicit TokenLexer(const std::vector<Token> &_tokens) : tokens(_tokens) {}

    std::optional<Token> getToken() override {
        if (tokens.empty() || currentTokenIdx >= tokens.size()) {
            return {};
        }
        return tokens[currentTokenIdx++];
    }

    std::string_view advanceStream(size_t characters) override { return ""; }

  private:
    const std::vector<Token> &tokens;
    int currentTokenIdx = 0;
};

} // namespace pdf
