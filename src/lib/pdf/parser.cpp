#include "parser.h"

#include <cstring>
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace pdf {

bool Parser::ensureTokensHaveBeenLexed() {
    if (currentTokenIdx < tokens.size()) {
        return true;
    }

    std::optional<Token> token = lexer.getToken();
    if (!token.has_value()) {
        return false;
    }
    if (token.value().type == Token::Type::INVALID) {
        return false;
    }

    tokens.push_back(token.value());
    return true;
}

bool Parser::currentTokenIs(Token::Type type) {
    if (!ensureTokensHaveBeenLexed()) {
        return false;
    }

    return tokens[currentTokenIdx].type == type;
}

Boolean *Parser::parseBoolean() {
    if (!currentTokenIs(Token::Type::BOOLEAN)) {
        return nullptr;
    }
    std::string_view &content = tokens[currentTokenIdx].content;

    currentTokenIdx++;

    bool value = false;
    if (content == "true") {
        value = true;
    } else if (content == "false") {
        value = false;
    } else {
        // TODO add logging
    }
    return new Boolean(content, value);
}

Integer *Parser::parseInteger() {
    if (!currentTokenIs(Token::Type::INTEGER)) {
        return nullptr;
    }

    auto &content = tokens[currentTokenIdx].content;
    try {
        // TODO is this conversion to a string really necessary?
        int64_t value = std::stoll(std::string(content));
        currentTokenIdx++;
        return new Integer(content, value);
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

    auto &content = tokens[currentTokenIdx].content;
    try {
        // TODO is this conversion to a string really necessary?
        double value = std::stod(std::string(content));
        currentTokenIdx++;
        return new Real(content, value);
    } catch (std::invalid_argument &) {
        // TODO add logging
    } catch (std::out_of_range &) {
        // TODO add logging
    }
    return nullptr;
}

Null *Parser::parseNullObject() {
    if (!currentTokenIs(Token::Type::NULL_OBJ)) {
        return nullptr;
    }

    auto &content = tokens[currentTokenIdx].content;
    currentTokenIdx++;
    return new Null(content);
}

LiteralString *Parser::parseLiteralString() {
    if (!currentTokenIs(Token::Type::LITERAL_STRING)) {
        return nullptr;
    }

    auto &content = tokens[currentTokenIdx].content;
    currentTokenIdx++;
    return new LiteralString(content);
}

HexadecimalString *Parser::parseHexadecimalString() {
    if (!currentTokenIs(Token::Type::HEXADECIMAL_STRING)) {
        return nullptr;
    }

    auto &content = tokens[currentTokenIdx].content;
    currentTokenIdx++;
    return new HexadecimalString(content.substr(1, content.size() - 2));
}

Name *Parser::parseName() {
    if (!currentTokenIs(Token::Type::NAME)) {
        return nullptr;
    }
    auto &content = tokens[currentTokenIdx].content;
    currentTokenIdx++;
    return new Name(content.substr(1));
}

Array *Parser::parseArray() {
    if (!currentTokenIs(Token::Type::ARRAY_START)) {
        return nullptr;
    }

    auto &objectStartContent = tokens[currentTokenIdx].content;
    auto beforeTokenIdx      = currentTokenIdx;
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

        ignoreNewLineTokens();

        objects.push_back(object);
    }

    auto &lastTokenContent = tokens[currentTokenIdx].content;
    currentTokenIdx++;
    auto tokenDiff  = lastTokenContent.data() - objectStartContent.data();
    auto dataLength = tokenDiff + lastTokenContent.size();
    auto data       = std::string_view(objectStartContent.data(), dataLength);
    return new Array(data, objects);
}

void Parser::ignoreNewLineTokens() {
    while (currentTokenIs(Token::Type::NEW_LINE)) {
        currentTokenIdx++;
    }
}

Dictionary *Parser::parseDictionary() {
    if (!currentTokenIs(Token::Type::DICTIONARY_START)) {
        return nullptr;
    }

    auto &objectStartContent = tokens[currentTokenIdx].content;
    auto beforeTokenIdx      = currentTokenIdx;
    currentTokenIdx++;

    if (currentTokenIs(Token::Type::NEW_LINE)) {
        currentTokenIdx++;
    }

    std::unordered_map<std::string, Object *> objects = {};
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

        ignoreNewLineTokens();

        // TODO is this conversion to a string really necessary?
        objects[std::string(key->value())] = value;
    }

    auto &lastTokenContent = tokens[currentTokenIdx].content;
    currentTokenIdx++;
    auto tokenDiff  = lastTokenContent.data() - objectStartContent.data();
    auto dataLength = tokenDiff + lastTokenContent.size();
    auto data       = std::string_view(objectStartContent.data(), dataLength);
    return new Dictionary(data, objects);
}

IndirectReference *Parser::parseIndirectReference() {
    if (!currentTokenIs(Token::Type::INDIRECT_REFERENCE)) {
        return nullptr;
    }

    const auto &content = tokens[currentTokenIdx].content;

    try {
        const size_t pos1 = content.find(' ');
        // TODO is this conversion to a string really necessary?
        const int64_t objectNumber     = std::stoll(std::string(content.substr(0, pos1)));
        const size_t pos2              = content.find(' ', pos1);
        const int64_t generationNumber = std::stoll(std::string(content.substr(pos1 + 1, pos2)));
        currentTokenIdx++;
        return new IndirectReference(content, objectNumber, generationNumber);
    } catch (std::out_of_range &err) {
        // TODO add logging
    } catch (std::invalid_argument &err) {
        // TODO add logging
    }
    return nullptr;
}

IndirectObject *Parser::parseIndirectObject() {
    if (!currentTokenIs(Token::Type::OBJECT_START)) {
        return nullptr;
    }

    auto objectStartContent = tokens[currentTokenIdx].content;
    auto beforeTokenIndex   = currentTokenIdx;
    currentTokenIdx++;

    ignoreNewLineTokens();

    auto object = parseObject();
    if (object == nullptr) {
        currentTokenIdx = beforeTokenIndex;
        return nullptr;
    }

    ignoreNewLineTokens();

    if (!currentTokenIs(Token::Type::OBJECT_END)) {
        currentTokenIdx = beforeTokenIndex;
        return nullptr;
    }

    try {
        const size_t pos1 = objectStartContent.find(' ');
        // TODO is this conversion to a string really necessary?
        const int64_t objectNumber     = std::stoll(std::string(objectStartContent.substr(0, pos1)));
        const size_t pos2              = objectStartContent.find(' ', pos1);
        const int64_t generationNumber = std::stoll(std::string(objectStartContent.substr(pos1 + 1, pos2)));

        auto &lastTokenContent = tokens[currentTokenIdx].content;
        currentTokenIdx++;
        auto tokenDiff  = lastTokenContent.data() - objectStartContent.data();
        auto dataLength = tokenDiff + lastTokenContent.size();
        auto data       = std::string_view(objectStartContent.data(), dataLength);
        return new IndirectObject(data, objectNumber, generationNumber, object);
    } catch (std::out_of_range &err) {
        // TODO add logging
    } catch (std::invalid_argument &err) {
        // TODO add logging
    }
    return nullptr;
}

Object *Parser::parseStreamOrDictionary() {
    if (!ensureTokensHaveBeenLexed()) {
        return nullptr;
    }

    auto objectStartContent = tokens[currentTokenIdx].content;
    auto beforeTokenIdx     = currentTokenIdx;
    auto dictionary         = parseDictionary();
    if (dictionary == nullptr) {
        currentTokenIdx = beforeTokenIdx;
        return nullptr;
    }

    if (currentTokenIs(Token::Type::NEW_LINE)) {
        currentTokenIdx++;
    }

    if (!currentTokenIs(Token::Type::STREAM_START)) {
        return dictionary;
    }

    currentTokenIdx++;

    if (!currentTokenIs(Token::Type::NEW_LINE)) {
        currentTokenIdx = beforeTokenIdx;
        return nullptr;
    }

    currentTokenIdx++;

    auto itr = dictionary->values.find("Length");
    if (itr == dictionary->values.end()) {
        // TODO add logging
        currentTokenIdx = beforeTokenIdx;
        return nullptr;
    }

    int64_t length = -1;
    if (itr->second->is<Integer>()) {
        length = itr->second->as<Integer>()->value;
    } else if (itr->second->is<IndirectReference>()) {
        auto obj = referenceResolver->resolve(itr->second->as<IndirectReference>());
        if (obj == nullptr) {
            TODO("add logging");
            return nullptr;
        }
        if (!obj->is<IndirectObject>()) {
            TODO("add logging");
            return nullptr;
        }
        if (!obj->as<IndirectObject>()->object->is<Integer>()) {
            TODO("add logging");
            return nullptr;
        }
        length = obj->as<IndirectObject>()->object->as<Integer>()->value;
    } else {
        TODO("add logging");
        currentTokenIdx = beforeTokenIdx;
        return nullptr;
    }

    auto streamData = lexer.advanceStream(length);
    if (currentTokenIs(Token::Type::NEW_LINE)) {
        currentTokenIdx++;
    }

    if (!currentTokenIs(Token::Type::STREAM_END)) {
        currentTokenIdx = beforeTokenIdx;
        return nullptr;
    }

    auto &lastTokenContent = tokens[currentTokenIdx].content;
    currentTokenIdx++;
    auto tokenDiff  = lastTokenContent.data() - objectStartContent.data();
    auto dataLength = tokenDiff + lastTokenContent.size();
    auto data       = std::string_view(objectStartContent.data(), dataLength);
    return new Stream(data, dictionary, streamData);
}

Object *Parser::parseObject() {
    ignoreNewLineTokens();

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

    auto nullObject = parseNullObject();
    if (nullObject != nullptr) {
        return nullObject;
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

    auto stream = parseStreamOrDictionary();
    if (stream != nullptr) {
        return stream;
    }

    auto indirectReference = parseIndirectReference();
    if (indirectReference != nullptr) {
        return indirectReference;
    }

    auto indirectObject = parseIndirectObject();
    if (indirectObject != nullptr) {
        return indirectObject;
    }

    return nullptr;
}

Object *Parser::parse() { return parseObject(); }

} // namespace pdf
