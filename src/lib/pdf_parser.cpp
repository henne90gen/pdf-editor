#include "pdf_parser.h"

#include <stdexcept>
namespace pdf{

bool Parser::currentTokenIs(Token::Type type) {
    if (currentTokenIdx >= tokens.size()) {
        std::optional<Token> token = lexer.getToken();
        if (!token.has_value()) {
            return false;
        }
        if (token.value().type == Token::Type::INVALID) {
            return false;
        }
        tokens.push_back(token.value());
    }

    return tokens[currentTokenIdx].type == type;
}

// bool Parser::currentTokenIs(Token::Type type) {
//     if (currentTokenIdx >= tokens.size()) {
//         std::optional<Token> token = lexer.getToken();
//         if (!token.has_value()) {
//             return false;
//         }
//         if (token.value().type == Token::Type::INVALID) {
//             return false;
//         }
//         tokens.push_back(token.value());
//     }
//
//     return tokens[currentTokenIdx].type == type;
// }

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

        while (currentTokenIs(Token::Type::NEW_LINE)) {
            // ignore NEW_LINE tokens
            currentTokenIdx++;
        }

        objects[key->value] = value;
    }

    currentTokenIdx++;
    return new Dictionary(objects);
}

IndirectReference *Parser::parseIndirectReference() {
    if (!currentTokenIs(Token::Type::INDIRECT_REFERENCE)) {
        return nullptr;
    }

    const std::string &content = tokens[currentTokenIdx].content;

    try {
        const size_t pos1              = content.find(' ');
        const int64_t objectNumber     = std::stoll(content.substr(0, pos1));
        const size_t pos2              = content.find(' ', pos1);
        const int64_t generationNumber = std::stoll(content.substr(pos1 + 1, pos2));
        currentTokenIdx++;
        return new IndirectReference(objectNumber, generationNumber);
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
    std::string objectStartContent = tokens[currentTokenIdx].content;

    auto beforeTokenIndex = currentTokenIdx;
    currentTokenIdx++;
    while (currentTokenIs(Token::Type::NEW_LINE)) {
        currentTokenIdx++;
    }

    auto object = parseObject();
    if (object == nullptr) {
        currentTokenIdx = beforeTokenIndex;
        return nullptr;
    }

    while (currentTokenIs(Token::Type::NEW_LINE)) {
        currentTokenIdx++;
    }

    if (!currentTokenIs(Token::Type::OBJECT_END)) {
        currentTokenIdx = beforeTokenIndex;
        return nullptr;
    }

    try {
        const size_t pos1              = objectStartContent.find(' ');
        const int64_t objectNumber     = std::stoll(objectStartContent.substr(0, pos1));
        const size_t pos2              = objectStartContent.find(' ', pos1);
        const int64_t generationNumber = std::stoll(objectStartContent.substr(pos1 + 1, pos2));
        currentTokenIdx++;
        return new IndirectObject(objectNumber, generationNumber, object);
    } catch (std::out_of_range &err) {
        // TODO add logging
    } catch (std::invalid_argument &err) {
        // TODO add logging
    }
    return nullptr;
}

Object *Parser::parseStreamOrDictionary() {
    auto beforeTokenIdx = currentTokenIdx;
    auto dictionary     = parseDictionary();
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
        currentTokenIdx=beforeTokenIdx;
        return nullptr;
    }

    currentTokenIdx++;

    auto itr = dictionary->values.find("Length");
    if (itr == dictionary->values.end()) {
        // TODO add logging
        currentTokenIdx = beforeTokenIdx;
        return nullptr;
    }

    if (itr->second->is<Integer>()) {
        auto length  = itr->second->as<Integer>()->value;
        auto content = lexer.advanceStream(length);
        if (currentTokenIs(Token::Type::NEW_LINE)) {
            currentTokenIdx++;
        }

        if (!currentTokenIs(Token::Type::STREAM_END)) {
            currentTokenIdx = beforeTokenIdx;
            return nullptr;
        }
        currentTokenIdx++;

        auto data = (char *)malloc(length);
        memcpy(data, content.c_str(), length);
        return new Stream(dictionary, data, length);
    } else if (itr->second->is<IndirectReference>()) {
        // TODO load indirect reference to length
        return nullptr;
    } else {
        // TODO add logging
        currentTokenIdx = beforeTokenIdx;
        return nullptr;
    }
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
}
