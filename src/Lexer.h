#pragma once

#include <optional>
#include <string>
#include <utility>

struct Token {
    enum class Type {
        BOOLEAN            = 0,
        INTEGER            = 1,
        REAL               = 2,
        LITERAL_STRING     = 3,
        HEXADECIMAL_STRING = 4,
        NAME               = 5,
        ARRAY_START        = 6,
        ARRAY_END          = 7,
        DICTIONARY_START   = 8,
        DICTIONARY_END     = 9,
        INDIRECT_REFERENCE = 10,
        NEW_LINE           = 11,
        INVALID            = 12,
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
