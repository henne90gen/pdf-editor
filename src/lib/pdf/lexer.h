#pragma once

#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace pdf {

struct Token {
    enum class Type {
        INVALID            = 0,
        NEW_LINE           = 1,
        BOOLEAN            = 2,
        INTEGER            = 3,
        REAL               = 4,
        LITERAL_STRING     = 5,
        HEXADECIMAL_STRING = 6,
        NAME               = 7,
        ARRAY_START        = 8,
        ARRAY_END          = 9,
        DICTIONARY_START   = 10,
        DICTIONARY_END     = 11,
        INDIRECT_REFERENCE = 12,
        OBJECT_START       = 13,
        OBJECT_END         = 14,
        STREAM_START       = 15,
        STREAM_END         = 16,
        NULL_OBJ           = 17,
        OPERATOR           = 18,
    };

    Type type;
    std::string content;
    Token(Type _type, std::string _content) : type(_type), content(std::move(_content)) {}
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
    virtual std::optional<Token> getToken()              = 0;
    virtual std::string advanceStream(size_t characters) = 0;
};

class TextLexer : public Lexer {
  public:
    explicit TextLexer(TextProvider &_textProvider) : textProvider(_textProvider) {}

    std::optional<Token> getToken() override;
    std::string advanceStream(size_t characters) override;

  private:
    TextProvider &textProvider;
    std::string currentWord;
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

    std::string advanceStream(size_t characters) override { return ""; }

  private:
    const std::vector<Token> &tokens;
    int currentTokenIdx = 0;
};

} // namespace pdf
