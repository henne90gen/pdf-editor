#include "pdf_lexer.h"

#include <iostream>
#include <regex>
#include <string>

namespace pdf {

std::string removeLeadingWhitespace(const std::string &str) {
    std::string result = str;
    while (!result.empty() && (result[0] == ' ' || result[0] == '\t')) {
        result = result.substr(1, result.length());
    }
    return result;
}

std::optional<Token> matchRegex(const std::string &word, const std::string &regex, Token::Type tokenType) {
    std::regex re(regex);
    auto itr = std::sregex_iterator(word.begin(), word.end(), re);
    if (itr != std::sregex_iterator()) {
        auto content = static_cast<std::string>((*itr).str());
        Token token  = {tokenType, content};
        return std::optional<Token>(token);
    }
    return {};
}

std::optional<Token> matchWordToken(const std::string &word) {
#define STARTS_WITH(word, search) word.find(search) == 0
    if (STARTS_WITH(word, "true")) {
        return Token(Token::Type::BOOLEAN, "true");
    }
    if (STARTS_WITH(word, "false")) {
        return Token(Token::Type::BOOLEAN, "false");
    }
    if (STARTS_WITH(word, "endobj")) {
        return Token(Token::Type::OBJECT_END, "endobj");
    }
    if (STARTS_WITH(word, "stream")) {
        return Token(Token::Type::STREAM_START, "stream");
    }
    if (STARTS_WITH(word, "endstream")) {
        return Token(Token::Type::STREAM_END, "endstream");
    }
    if (STARTS_WITH(word, "null")) {
        return Token(Token::Type::NULL_OBJ, "null");
    }
    return {};
}

std::optional<Token> matchCharToken(const std::string &word) {
    if (!word.empty() && word[0] == '\n') {
        return Token(Token::Type::NEW_LINE, "\n");
    }
    if (!word.empty() && word[0] == '[') {
        return Token(Token::Type::ARRAY_START, "[");
    }
    if (!word.empty() && word[0] == ']') {
        return Token(Token::Type::ARRAY_END, "]");
    }
    if (STARTS_WITH(word, "<<")) {
        return Token(Token::Type::DICTIONARY_START, "<<");
    }
    if (STARTS_WITH(word, ">>")) {
        return Token(Token::Type::DICTIONARY_END, ">>");
    }
    return {};
}

std::optional<Token> matchString(const std::string &word) {
    if (word[0] != '(') {
        return {};
    }

    // TODO implement backslash handling

    int openParenthesis = 1;
    int stringLength    = -1;
    for (int i = 1; i < word.size(); i++) {
        if (word[i] == '(') {
            openParenthesis++;
        } else if (word[i] == ')') {
            openParenthesis--;
        }
        if (openParenthesis == 0) {
            stringLength = i;
            break;
        }
    }

    return Token(Token::Type::LITERAL_STRING, word.substr(0, stringLength + 1));
}

std::optional<Token> findToken(const std::string &word) {
    auto indirectReferenceToken = matchRegex(word, "^[0-9]+ [0-9]+ R", Token::Type::INDIRECT_REFERENCE);
    if (indirectReferenceToken.has_value()) {
        return indirectReferenceToken.value();
    }

    auto objectStartToken = matchRegex(word, "^[0-9]+ [0-9]+ obj", Token::Type::OBJECT_START);
    if (objectStartToken.has_value()) {
        return objectStartToken.value();
    }

    auto floatToken = matchRegex(word, "^[+-]?[0-9]+\\.[0-9]+", Token::Type::REAL);
    if (floatToken.has_value()) {
        return floatToken.value();
    }

    auto intToken = matchRegex(word, "^[+-]?[0-9]+", Token::Type::INTEGER);
    if (intToken.has_value()) {
        return intToken.value();
    }

    auto wordToken = matchWordToken(word);
    if (wordToken.has_value()) {
        return wordToken.value();
    }

    auto hexadecimalString = matchRegex(word, "^<[0-9a-fA-F]*>", Token::Type::HEXADECIMAL_STRING);
    if (hexadecimalString.has_value()) {
        return hexadecimalString.value();
    }

    auto literalString = matchString(word);
    if (literalString.has_value()) {
        return literalString;
    }

    // TODO check with the standard again
    auto nameToken = matchRegex(word, R"(^\/[^\r\n\t\f\v /<>\[\]\(\)\{\}]*)", Token::Type::NAME);
    if (nameToken.has_value()) {
        return nameToken.value();
    }

    auto charToken = matchCharToken(word);
    if (charToken.has_value()) {
        return charToken.value();
    }

    return {};
}

std::optional<Token> Lexer::getToken() {
    std::string previousWord = currentWord;
    while (true) {
        if (currentWord.empty()) {
            auto optionalCode = textProvider.getText();
            if (!optionalCode.has_value()) {
                break;
            }
            currentWord = optionalCode.value();
            if (currentWord.empty()) {
                continue;
            }
        }

        currentWord = removeLeadingWhitespace(currentWord);

        auto token = findToken(currentWord);
        if (token.has_value()) {
            currentWord = currentWord.substr(token.value().content.length(), currentWord.length() - 1);
            return token;
        }

        if (currentWord == previousWord) {
            break;
        }
        previousWord = currentWord;
    }

    if (!currentWord.empty()) {
        std::cerr << "Found an invalid token: '" + currentWord + "'" << std::endl;
        return std::optional(Token(Token::Type::INVALID, currentWord));
    }

    return {};
}

std::string Lexer::advanceStream(size_t characters) {
    std::string tmp = currentWord.substr(0, characters);
    currentWord     = currentWord.substr(characters);
    return tmp;
}

} // namespace pdf
