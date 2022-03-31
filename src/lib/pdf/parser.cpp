#include "parser.h"

#include <cstring>
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace pdf {

NoopReferenceResolver GlobalNoopReferenceResolver = {};

Parser::Parser(Lexer &_lexer, util::Allocator &_allocator)
    : lexer(_lexer), allocator(_allocator), referenceResolver(&GlobalNoopReferenceResolver) {}

Parser::Parser(Lexer &_lexer, util::Allocator &_allocator, ReferenceResolver *_referenceResolver)
    : lexer(_lexer), allocator(_allocator), referenceResolver(_referenceResolver) {}

bool Parser::ensure_tokens_have_been_lexed() {
    if (currentTokenIdx < tokens.size()) {
        return true;
    }

    std::optional<Token> token = lexer.get_token();
    if (!token.has_value()) {
        return false;
    }
    if (token.value().type == Token::Type::INVALID) {
        return false;
    }

    tokens.push_back(token.value());
    return true;
}

bool Parser::current_token_is(Token::Type type) {
    if (!ensure_tokens_have_been_lexed()) {
        return false;
    }

    return tokens[currentTokenIdx].type == type;
}

Boolean *Parser::parse_boolean() {
    if (!current_token_is(Token::Type::BOOLEAN)) {
        return nullptr;
    }
    auto content = tokens[currentTokenIdx].content;

    currentTokenIdx++;

    bool value = false;
    if (content == "true") {
        value = true;
    } else if (content == "false") {
        value = false;
    } else {
        // TODO add logging
        return nullptr;
    }
    return allocator.allocate<Boolean>(value);
}

Integer *Parser::parse_integer() {
    if (!current_token_is(Token::Type::INTEGER)) {
        return nullptr;
    }

    auto content = tokens[currentTokenIdx].content;
    try {
        // TODO is this conversion to a string really necessary?
        int64_t value = std::stoll(std::string(content));
        currentTokenIdx++;
        return allocator.allocate<Integer>(value);
    } catch (std::invalid_argument &) {
        // TODO add logging
    } catch (std::out_of_range &) {
        // TODO add logging
    }
    return nullptr;
}

Real *Parser::parse_real() {
    if (!current_token_is(Token::Type::REAL)) {
        return nullptr;
    }

    auto content = tokens[currentTokenIdx].content;
    try {
        // TODO is this conversion to a string really necessary?
        double value = std::stod(std::string(content));
        currentTokenIdx++;
        return allocator.allocate<Real>(value);
    } catch (std::invalid_argument &) {
        // TODO add logging
    } catch (std::out_of_range &) {
        // TODO add logging
    }
    return nullptr;
}

Null *Parser::parse_null_object() {
    if (!current_token_is(Token::Type::NULL_OBJ)) {
        return nullptr;
    }

    currentTokenIdx++;
    return allocator.allocate<Null>();
}

LiteralString *Parser::parse_literal_string() {
    if (!current_token_is(Token::Type::LITERAL_STRING)) {
        return nullptr;
    }

    auto content = tokens[currentTokenIdx].content;
    currentTokenIdx++;
    return allocator.allocate<LiteralString>(std::string(content.substr(1, content.size() - 2)));
}

HexadecimalString *Parser::parse_hexadecimal_string() {
    if (!current_token_is(Token::Type::HEXADECIMAL_STRING)) {
        return nullptr;
    }

    auto content = tokens[currentTokenIdx].content;
    currentTokenIdx++;
    return allocator.allocate<HexadecimalString>(std::string(content.substr(1, content.size() - 2)));
}

Name *Parser::parse_name() {
    if (!current_token_is(Token::Type::NAME)) {
        return nullptr;
    }

    auto content = tokens[currentTokenIdx].content;
    currentTokenIdx++;
    return allocator.allocate<Name>(std::string(content.substr(1)));
}

Array *Parser::parse_array() {
    if (!current_token_is(Token::Type::ARRAY_START)) {
        return nullptr;
    }

    auto objectStartContent = tokens[currentTokenIdx].content;
    auto beforeTokenIdx     = currentTokenIdx;
    currentTokenIdx++;

    std::vector<Object *> objects = {};
    while (true) {
        if (current_token_is(Token::Type::ARRAY_END)) {
            break;
        }
        auto object = parse();
        if (object == nullptr) {
            currentTokenIdx = beforeTokenIdx;
            return nullptr;
        }

        ignore_new_line_tokens();

        objects.push_back(object);
    }

    auto lastTokenContent = tokens[currentTokenIdx].content;
    currentTokenIdx++;
    auto tokenDiff  = lastTokenContent.data() - objectStartContent.data();
    auto dataLength = tokenDiff + lastTokenContent.size();
    auto data       = std::string_view(objectStartContent.data(), dataLength);
    return allocator.allocate<Array>(objects);
}

void Parser::ignore_new_line_tokens() {
    while (current_token_is(Token::Type::NEW_LINE)) {
        currentTokenIdx++;
    }
}

Dictionary *Parser::parse_dictionary() {
    if (!current_token_is(Token::Type::DICTIONARY_START)) {
        return nullptr;
    }

    auto beforeTokenIdx = currentTokenIdx;
    currentTokenIdx++;

    ignore_new_line_tokens();

    std::unordered_map<std::string, Object *> objects = {};
    while (!current_token_is(Token::Type::DICTIONARY_END)) {
        auto key = parse_name();
        if (key == nullptr) {
            currentTokenIdx = beforeTokenIdx;
            return nullptr;
        }

        auto value = parse();
        if (value == nullptr) {
            currentTokenIdx = beforeTokenIdx;
            return nullptr;
        }

        ignore_new_line_tokens();

        objects[key->value] = value;
    }

    currentTokenIdx++;
    return allocator.allocate<Dictionary>(std::move(objects));
}

IndirectReference *Parser::parse_indirect_reference() {
    if (!current_token_is(Token::Type::INDIRECT_REFERENCE)) {
        return nullptr;
    }

    const auto content = tokens[currentTokenIdx].content;

    try {
        const size_t pos1 = content.find(' ');
        // TODO is this conversion to a string really necessary?
        const int64_t objectNumber     = std::stoll(std::string(content.substr(0, pos1)));
        const size_t pos2              = content.find(' ', pos1);
        const int64_t generationNumber = std::stoll(std::string(content.substr(pos1 + 1, pos2)));
        currentTokenIdx++;
        return allocator.allocate<IndirectReference>(objectNumber, generationNumber);
    } catch (std::out_of_range &err) {
        // TODO add logging
    } catch (std::invalid_argument &err) {
        // TODO add logging
    }
    return nullptr;
}

IndirectObject *Parser::parse_indirect_object() {
    if (!current_token_is(Token::Type::OBJECT_START)) {
        return nullptr;
    }

    auto objectStartContent = tokens[currentTokenIdx].content;
    auto beforeTokenIndex   = currentTokenIdx;
    currentTokenIdx++;

    ignore_new_line_tokens();

    auto object = parse();
    if (object == nullptr) {
        currentTokenIdx = beforeTokenIndex;
        return nullptr;
    }

    ignore_new_line_tokens();

    if (!current_token_is(Token::Type::OBJECT_END)) {
        currentTokenIdx = beforeTokenIndex;
        return nullptr;
    }

    try {
        const size_t pos1 = objectStartContent.find(' ');
        // TODO is this conversion to a string really necessary?
        const int64_t objectNumber     = std::stoll(std::string(objectStartContent.substr(0, pos1)));
        const size_t pos2              = objectStartContent.find(' ', pos1);
        const int64_t generationNumber = std::stoll(std::string(objectStartContent.substr(pos1 + 1, pos2)));

        auto lastTokenContent = tokens[currentTokenIdx].content;
        currentTokenIdx++;
        auto tokenDiff  = lastTokenContent.data() - objectStartContent.data();
        auto dataLength = tokenDiff + lastTokenContent.size();
        auto data       = std::string_view(objectStartContent.data(), dataLength);
        return allocator.allocate<IndirectObject>(objectNumber, generationNumber, object);
    } catch (std::out_of_range &err) {
        // TODO add logging
    } catch (std::invalid_argument &err) {
        // TODO add logging
    }
    return nullptr;
}

Object *Parser::parse_stream_or_dictionary() {
    if (!ensure_tokens_have_been_lexed()) {
        return nullptr;
    }

    auto objectStartContent = tokens[currentTokenIdx].content.data();
    auto beforeTokenIdx     = currentTokenIdx;
    auto dictionary         = parse_dictionary();
    if (dictionary == nullptr) {
        currentTokenIdx = beforeTokenIdx;
        return nullptr;
    }

    ignore_new_line_tokens();

    if (!current_token_is(Token::Type::STREAM_START)) {
        return dictionary;
    }
    currentTokenIdx++;

    if (!current_token_is(Token::Type::NEW_LINE)) {
        currentTokenIdx = beforeTokenIdx;
        dictionary->~Dictionary();
        return nullptr;
    }
    currentTokenIdx++;

    auto itr = dictionary->values.find("Length");
    if (itr == dictionary->values.end()) {
        // TODO add logging
        currentTokenIdx = beforeTokenIdx;
        dictionary->~Dictionary();
        return nullptr;
    }

    int64_t length = -1;
    auto &value    = itr->second;
    if (value->is<Integer>()) {
        length = value->as<Integer>()->value;
    } else if (value->is<IndirectReference>()) {
        auto obj = referenceResolver->resolve(value->as<IndirectReference>());
        if (obj == nullptr) {
            // TODO add logging
            dictionary->~Dictionary();
            return nullptr;
        }
        if (!obj->is<IndirectObject>()) {
            // TODO add logging
            dictionary->~Dictionary();
            return nullptr;
        }
        if (!obj->as<IndirectObject>()->object->is<Integer>()) {
            // TODO add logging
            dictionary->~Dictionary();
            return nullptr;
        }
        length = obj->as<IndirectObject>()->object->as<Integer>()->value;
    } else {
        // TODO add logging
        currentTokenIdx = beforeTokenIdx;
        dictionary->~Dictionary();
        return nullptr;
    }

    auto streamData = lexer.advance_stream(length);
    ignore_new_line_tokens();

    if (!current_token_is(Token::Type::STREAM_END)) {
        currentTokenIdx = beforeTokenIdx;
        dictionary->~Dictionary();
        return nullptr;
    }

    auto lastTokenContent = tokens[currentTokenIdx].content;
    currentTokenIdx++;
    auto tokenDiff  = lastTokenContent.data() - objectStartContent;
    auto dataLength = tokenDiff + lastTokenContent.size();
    auto data       = std::string_view(objectStartContent, dataLength);
    return allocator.allocate<Stream>(dictionary, streamData);
}

Object *Parser::parse() {
    ignore_new_line_tokens();

    auto boolean = parse_boolean();
    if (boolean != nullptr) {
        return boolean;
    }

    auto integer = parse_integer();
    if (integer != nullptr) {
        return integer;
    }

    auto real = parse_real();
    if (real != nullptr) {
        return real;
    }

    auto nullObject = parse_null_object();
    if (nullObject != nullptr) {
        return nullObject;
    }

    auto literalString = parse_literal_string();
    if (literalString != nullptr) {
        return literalString;
    }

    auto hexadecimalString = parse_hexadecimal_string();
    if (hexadecimalString != nullptr) {
        return hexadecimalString;
    }

    auto name = parse_name();
    if (name != nullptr) {
        return name;
    }

    auto array = parse_array();
    if (array != nullptr) {
        return array;
    }

    auto stream = parse_stream_or_dictionary();
    if (stream != nullptr) {
        return stream;
    }

    auto indirectReference = parse_indirect_reference();
    if (indirectReference != nullptr) {
        return indirectReference;
    }

    auto indirectObject = parse_indirect_object();
    if (indirectObject != nullptr) {
        return indirectObject;
    }

    return nullptr;
}

} // namespace pdf
