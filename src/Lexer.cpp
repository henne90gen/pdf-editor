#include "Lexer.h"

#include <iostream>
#include <regex>
#include <string>

std::string removeLeadingWhitespace(const std::string &str) {
    std::string result = str;
    while (!result.empty() && (result[0] == ' ' || result[0] == '\t')) {
        result = result.substr(1, result.length());
    }
    return result;
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

        auto indirectReferenceToken = matchRegex("^[0-9]+ [0-9]+ R", Token::Type::INDIRECT_REFERENCE);
        if (indirectReferenceToken.has_value()) {
            currentWord = currentWord.substr(indirectReferenceToken.value().content.length(), currentWord.length() - 1);
            return indirectReferenceToken.value();
        }

        auto objectStartToken = matchRegex("^[0-9]+ [0-9]+ obj", Token::Type::OBJECT_START);
        if (objectStartToken.has_value()) {
            currentWord = currentWord.substr(objectStartToken.value().content.length(), currentWord.length() - 1);
            return objectStartToken.value();
        }

        auto floatToken = matchRegex("^[+-]?[0-9]+\\.[0-9]+", Token::Type::REAL);
        if (floatToken.has_value()) {
            currentWord = currentWord.substr(floatToken.value().content.length(), currentWord.length() - 1);
            return floatToken.value();
        }

        auto intToken = matchRegex("^[+-]?[0-9]+", Token::Type::INTEGER);
        if (intToken.has_value()) {
            currentWord = currentWord.substr(intToken.value().content.length(), currentWord.length() - 1);
            return intToken.value();
        }

        auto wordToken = matchWordToken();
        if (wordToken.has_value()) {
            currentWord = currentWord.substr(wordToken.value().content.length(), currentWord.length() - 1);
            return wordToken.value();
        }

        auto hexadecimalString = matchRegex("^<[0-9a-fA-F]*>", Token::Type::HEXADECIMAL_STRING);
        if (hexadecimalString.has_value()) {
            currentWord = currentWord.substr(hexadecimalString.value().content.length(), currentWord.length() - 1);
            return hexadecimalString.value();
        }

        // TODO check with the standard again
        auto nameToken = matchRegex(R"(^\/[^\r\n\t\f\v \[\]\(\)\{\}]*)", Token::Type::NAME);
        if (nameToken.has_value()) {
            // TODO parse content of name token and replace '#__' with the corresponding characters
            currentWord = currentWord.substr(nameToken.value().content.length(), currentWord.length() - 1);
            return nameToken.value();
        }

        auto charToken = matchCharToken();
        if (charToken.has_value()) {
            currentWord = currentWord.substr(charToken.value().content.length(), currentWord.length() - 1);
            return charToken.value();
        }

        if (currentWord == previousWord) {
            break;
        }
        previousWord = currentWord;
    }

    std::string invalidToken;
    auto nextSpace = currentWord.find(' ');
    if (nextSpace == std::string::npos) {
        invalidToken = currentWord;
        currentWord  = "";
    } else {
        invalidToken = currentWord.substr(0, nextSpace);
        currentWord  = currentWord.substr(nextSpace + 1, currentWord.length() - 1);
    }

    if (!invalidToken.empty()) {
        std::cerr << "Found an invalid token: '" + invalidToken + "'";
        return std::optional(Token(Token::Type::INVALID, invalidToken));
    }

    return {};
}

std::optional<Token> Lexer::matchRegex(const std::string &regex, Token::Type tokenType) {
    std::regex re(regex);
    auto itr = std::sregex_iterator(currentWord.begin(), currentWord.end(), re);
    if (itr != std::sregex_iterator()) {
        auto content = static_cast<std::string>((*itr).str());
        Token token  = {tokenType, content};
        return std::optional<Token>(token);
    }
    return {};
}

std::optional<Token> Lexer::matchWordToken() {
#define STARTS_WITH(word, search) word.find(search) == 0
    if (STARTS_WITH(currentWord, "true")) {
        return Token(Token::Type::BOOLEAN, "true");
    }
    if (STARTS_WITH(currentWord, "false")) {
        return Token(Token::Type::BOOLEAN, "false");
    }
    if (STARTS_WITH(currentWord, "endobj")) {
        return Token(Token::Type::OBJECT_END, "endobj");
    }
    return {};
}

std::optional<Token> Lexer::matchCharToken() {
    if (!currentWord.empty() && currentWord[0] == '\n') {
        return Token(Token::Type::NEW_LINE, "\n");
    }
    if (!currentWord.empty() && currentWord[0] == '[') {
        return Token(Token::Type::ARRAY_START, "[");
    }
    if (!currentWord.empty() && currentWord[0] == ']') {
        return Token(Token::Type::ARRAY_END, "]");
    }
    if (STARTS_WITH(currentWord, "<<")) {
        return Token(Token::Type::DICTIONARY_START, "<<");
    }
    if (STARTS_WITH(currentWord, ">>")) {
        return Token(Token::Type::DICTIONARY_END, ">>");
    }
    return {};
}
