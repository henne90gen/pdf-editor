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

LiteralString *Parser::parseLiteralString() {
    if (!currentTokenIs(Token::Type::LITERAL_STRING)) {
        return nullptr;
    }
    std::string &content = tokens[currentTokenIdx].content;
    currentTokenIdx++;
    return new LiteralString(content.substr(1, content.size() - 2));
}

HexadecimalString *Parser::parseHexadecimalString() {
    if (!currentTokenIs(Token::Type::HEXADECIMAL_STRING)) {
        return nullptr;
    }
    std::string &content = tokens[currentTokenIdx].content;
    currentTokenIdx++;
    return new HexadecimalString(content.substr(1, content.size() - 2));
}

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

        while (currentTokenIs(Token::Type::NEW_LINE)) {
            // ignore NEW_LINE tokens
            currentTokenIdx++;
        }

        objects.push_back(object);
    }

    currentTokenIdx++;
    return new Array(objects);
}

Dictionary *Parser::parseDictionary() {
    if (!currentTokenIs(Token::Type::DICTIONARY_START)) {
        return nullptr;
    }

    auto beforeTokenIdx = currentTokenIdx;
    currentTokenIdx++;

    std::unordered_map<Name *, Object *> objects = {};
    while (true) {
        if (currentTokenIs(Token::Type::DICTIONARY_END)) {
            break;
        }

        auto key = parseName();
        if (key == nullptr) {
            currentTokenIdx = beforeTokenIdx;
            return nullptr;
        }

        auto value = parseObject();
        if (value == nullptr) {
            currentTokenIdx = beforeTokenIdx;
            return nullptr;
        }

        while (currentTokenIs(Token::Type::NEW_LINE)) {
            // ignore NEW_LINE tokens
            currentTokenIdx++;
        }

        objects[key] = value;
    }

    currentTokenIdx++;
    return new Dictionary(objects);
}

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

    auto literalString = parseLiteralString();
    if (literalString != nullptr) {
        return literalString;
    }

    auto hexadecimalString = parseHexadecimalString();
    if (hexadecimalString != nullptr) {
        return hexadecimalString;
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

    auto indirectReference = parseIndirectReference();
    if (indirectReference != nullptr) {
        return indirectReference;
    }

    return nullptr;
}

Object *Parser::parse() {
    while (true) {
        std::optional<Token> token = lexer.getToken();
        if (!token.has_value()) {
            break;
        }
        if (token.value().type == Token::Type::INVALID) {
            return nullptr;
        }
        tokens.push_back(token.value());
    }

    return parseObject();
}

IndirectReference *Parser::parseIndirectReference() {
    if (!currentTokenIs(Token::Type::INDIRECT_REFERENCE)) {
        return nullptr;
    }

    const std::string &content = tokens[currentTokenIdx].content;

    try {
        const size_t pos1            = content.find(' ');
        const int64_t typeId         = std::stoll(content.substr(0, pos1));
        const size_t pos2            = content.find(' ', pos1);
        const int64_t revisionNumber = std::stoll(content.substr(pos1 + 1, pos2));
        currentTokenIdx++;
        return new IndirectReference(typeId, revisionNumber);
    } catch (std::out_of_range &err) {
        // TODO add logging
    } catch (std::invalid_argument &err) {
        // TODO add logging
    }
    return nullptr;
}
