#include "Parser.h"

#include <stdexcept>

bool Parser::currentTokenIs(Token::Type type) const {
    return currentTokenIdx < tokens.size() && tokens[currentTokenIdx].type == type;
}

Boolean *Parser::parseBoolean() {
    if (!currentTokenIs(Token::Type::BOOLEAN)) {
        return nullptr;
    }
    std::string &content = tokens[currentTokenIdx].content;

    currentTokenIdx++;

    bool value = false;
    if (content == "true") {
        value = true;
    } else if (content == "false") {
        value = false;
    } else {
        // TODO add logging
    }
    return new Boolean(value);
}

Integer *Parser::parseInteger() {
    if (!currentTokenIs(Token::Type::INTEGER)) {
        return nullptr;
    }
    std::string &content = tokens[currentTokenIdx].content;
    try {
        int64_t value = std::stoll(content);
        currentTokenIdx++;
        return new Integer(value);
    } catch (std::invalid_argument &) {
        // TODO add logging
    } catch (std::out_of_range &) {
        // TODO add logging
    }
    return nullptr;
}

Real *Parser::parseReal() {
    if (!currentTokenIs(Token::Type::REAL)) {
        return nullptr;
    }
    std::string &content = tokens[currentTokenIdx].content;
    try {
        double value = std::stod(content);
        currentTokenIdx++;
        return new Real(value);
    } catch (std::invalid_argument &) {
        // TODO add logging
    } catch (std::out_of_range &) {
        // TODO add logging
    }
    return nullptr;
}

String *Parser::parseString() { return nullptr; }

Name *Parser::parseName() {
    if (!currentTokenIs(Token::Type::NAME)) {
        return nullptr;
    }
    std::string &content = tokens[currentTokenIdx].content;
    currentTokenIdx++;
    return new Name(content.substr(1));
}

Array *Parser::parseArray() {
    if (!currentTokenIs(Token::Type::ARRAY_START)) {
        return nullptr;
    }

    auto beforeTokenIdx = currentTokenIdx;
    currentTokenIdx++;

    std::vector<Object *> objects = {};
    while (true) {
        if (currentTokenIs(Token::Type::ARRAY_END)) {
            break;
        }
        auto object = parseObject();
        if (object == nullptr) {
            currentTokenIdx = beforeTokenIdx;
            return nullptr;
        }
        objects.push_back(object);
    }

    currentTokenIdx++;
    return new Array(objects);
}

Dictionary *Parser::parseDictionary() { return nullptr; }

Object *Parser::parseObject() {
    auto boolean = parseBoolean();
    if (boolean != nullptr) {
        return boolean;
    }

    auto integer = parseInteger();
    if (integer != nullptr) {
        return integer;
    }

    auto real = parseReal();
    if (real != nullptr) {
        return real;
    }

    auto string = parseString();
    if (string != nullptr) {
        return string;
    }

    auto name = parseName();
    if (name != nullptr) {
        return name;
    }

    auto array = parseArray();
    if (array != nullptr) {
        return array;
    }

    auto dictionary = parseDictionary();
    if (dictionary != nullptr) {
        return dictionary;
    }

    return nullptr;
}

Object *Parser::parse() {
    while (true) {
        std::optional<Token> token = lexer.getToken();
        if (!token.has_value()) {
            break;
        }
        tokens.push_back(token.value());
    }

    return parseObject();
}
