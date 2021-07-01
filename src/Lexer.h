#pragma once

#include <optional>
#include <string>
#include <utility>

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
        STREAM_START       = 13,
        STREAM_END         = 14,
    };

    Type type;
    std::string content;
    Token(Type _type, std::string _content) : type(_type), content(std::move(_content)) {}
};

class TextProvider {
  public:
    virtual std::optional<std::string> getText() = 0;
};

class StringTextProvider : public TextProvider {
  public:
    explicit StringTextProvider(std::string _text) : text(std::move(_text)) {}

    std::optional<std::string> getText() override {
        if (hasBeenQueried) {
            return {};
        }
        hasBeenQueried = true;
        return std::optional(text);
    }

  private:
    std::string text;
    bool hasBeenQueried = false;
};

class Lexer {
  public:
    explicit Lexer(TextProvider &_textProvider) : textProvider(_textProvider) {}

    std::optional<Token> getToken();

  private:
    std::optional<Token> matchRegex(const std::string &regex, Token::Type tokenType);
    std::optional<Token> matchWordToken();
    std::optional<Token> matchCharToken();

  private:
    TextProvider &textProvider;
    std::string currentWord;
};
