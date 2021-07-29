#include "lexer.h"

#include <iostream>
#include <regex>
#include <string>

namespace pdf {

// NOTE pre-computing the regex increased performance 15x
// NOTE moving regex evaluation to the back of findToken increased performance 2x
auto indirectReferenceRegex = std::regex("^[0-9]+ [0-9]+ R");
auto objectStartRegex       = std::regex("^[0-9]+ [0-9]+ obj");
auto floatRegex             = std::regex("^[+-]?[0-9]+\\.[0-9]+");
auto intRegex               = std::regex("^[+-]?[0-9]+");
auto hexadecimalRegex       = std::regex("^<[0-9a-fA-F]*>");
auto nameRegex              = std::regex(R"(^\/[^\r\n\t\f\v /<>\[\]\(\)\{\}]*)");

std::string_view removeLeadingWhitespace(const std::string_view &str) {
    std::string_view result = str;
    while (!result.empty() && (result[0] == ' ' || result[0] == '\t')) {
        result = result.substr(1, result.length());
    }
    return result;
}

std::optional<Token> matchRegex(const std::string_view &word, const std::regex &regex, Token::Type tokenType) {
    // TODO improve this, it is probably very inefficient
    auto tmp = std::string(word);
    auto itr = std::sregex_iterator(tmp.begin(), tmp.end(), regex);
    if (itr != std::sregex_iterator()) {
        auto content = static_cast<std::string>((*itr).str());
        // TODO I hope this ensures that we still point to the original data stream
        auto token = Token(tokenType, word.substr(0, content.length()));
        return std::optional<Token>(token);
    }
    return {};
}

std::optional<Token> matchWordToken(const std::string_view &word) {
    // TODO combine common prefixes for better performance (begin, end)
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
    if (STARTS_WITH(word, "begincmap")) {
        return Token(Token::Type::CMAP_BEGIN, "begincmap");
    }
    if (STARTS_WITH(word, "endcmap")) {
        return Token(Token::Type::CMAP_END, "endcmap");
    }
    if (STARTS_WITH(word, "usecmap")) {
        return Token(Token::Type::CMAP_USE, "usecmap");
    }
    if (STARTS_WITH(word, "begincodespacerange")) {
        return Token(Token::Type::CMAP_BEGIN_CODE_SPACE_RANGE, "begincodespacerange");
    }
    if (STARTS_WITH(word, "endcodespacerange")) {
        return Token(Token::Type::CMAP_END_CODE_SPACE_RANGE, "endcodespacerange");
    }
    if (STARTS_WITH(word, "usefont")) {
        return Token(Token::Type::CMAP_USE_FONT, "usefont");
    }
    if (STARTS_WITH(word, "beginbfchar")) {
        return Token(Token::Type::CMAP_BEGIN_BF_CHAR, "beginbfchar");
    }
    if (STARTS_WITH(word, "endbfchar")) {
        return Token(Token::Type::CMAP_END_BF_CHAR, "endbfchar");
    }
    if (STARTS_WITH(word, "beginbfrange")) {
        return Token(Token::Type::CMAP_BEGIN_BF_RANGE, "beginbfrange");
    }
    if (STARTS_WITH(word, "endbfrange")) {
        return Token(Token::Type::CMAP_END_BF_RANGE, "endbfrange");
    }
    if (STARTS_WITH(word, "begincidchar")) {
        return Token(Token::Type::CMAP_BEGIN_CID_CHAR, "begincidchar");
    }
    if (STARTS_WITH(word, "endcidchar")) {
        return Token(Token::Type::CMAP_END_CID_CHAR, "endcidchar");
    }
    if (STARTS_WITH(word, "begincidrange")) {
        return Token(Token::Type::CMAP_BEGIN_CID_RANGE, "begincidrange");
    }
    if (STARTS_WITH(word, "endcidrange")) {
        return Token(Token::Type::CMAP_END_CID_RANGE, "endcidrange");
    }
    if (STARTS_WITH(word, "beginnotdefchar")) {
        return Token(Token::Type::CMAP_BEGIN_NOTDEF_CHAR, "beginnotdefchar");
    }
    if (STARTS_WITH(word, "endnotdefchar")) {
        return Token(Token::Type::CMAP_END_NOTDEF_CHAR, "endnotdefchar");
    }
    if (STARTS_WITH(word, "beginnotdefrange")) {
        return Token(Token::Type::CMAP_BEGIN_NOTDEF_RANGE, "beginnotdefrange");
    }
    if (STARTS_WITH(word, "endnotdefrange")) {
        return Token(Token::Type::CMAP_END_NOTDEF_RANGE, "endnotdefrange");
    }
    return {};
}

std::optional<Token> matchCharToken(const std::string_view &word) {
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

std::optional<Token> matchString(const std::string_view &word) {
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

std::optional<Token> matchOperator(const std::string_view &word) {
    std::vector<std::string> operators = {
          // Text Operators
          "BT", "ET", "Td", "TD", "Tm", "T*", "Tc", "Tw", "Tz", "TL", "Tf", "Tr", "Ts",
          // Graphics Operators
          "q", "Q", "cm", "w", "J", "j", "M", "d", "ri", "i", "gs",
          // Path Construction Operators
          "m", "l", "c", "v", "y", "h", "re",
          // Path Painting Operators
          "S", "s", "f*", "F", "f", "B*", "B", "b*", "b", "n",
          // Clipping Path Operators
          "W*", "W",
          // Unsorted Operators
          "Tj", "TJ", "d0", "d1", "CS", "SC", "SCN", "sc", "scn", "G", "g", "RG", "rg", "K",
          "k", //
    };
    for (auto &op : operators) {
        if (STARTS_WITH(word, op)) {
            return Token(Token::Type::OPERATOR, word.substr(0, op.size()));
        }
    }
    return {};
}

std::optional<Token> findToken(const std::string_view &word) {
    auto literalString = matchString(word);
    if (literalString.has_value()) {
        return literalString;
    }

    auto charToken = matchCharToken(word);
    if (charToken.has_value()) {
        return charToken.value();
    }

    auto wordToken = matchWordToken(word);
    if (wordToken.has_value()) {
        return wordToken.value();
    }

    auto operatorToken = matchOperator(word);
    if (operatorToken.has_value()) {
        return operatorToken.value();
    }

    auto indirectReferenceToken = matchRegex(word, indirectReferenceRegex, Token::Type::INDIRECT_REFERENCE);
    if (indirectReferenceToken.has_value()) {
        return indirectReferenceToken.value();
    }

    auto objectStartToken = matchRegex(word, objectStartRegex, Token::Type::OBJECT_START);
    if (objectStartToken.has_value()) {
        return objectStartToken.value();
    }

    auto floatToken = matchRegex(word, floatRegex, Token::Type::REAL);
    if (floatToken.has_value()) {
        return floatToken.value();
    }

    auto intToken = matchRegex(word, intRegex, Token::Type::INTEGER);
    if (intToken.has_value()) {
        return intToken.value();
    }

    auto hexadecimalString = matchRegex(word, hexadecimalRegex, Token::Type::HEXADECIMAL_STRING);
    if (hexadecimalString.has_value()) {
        return hexadecimalString.value();
    }

    // TODO check with the standard again
    auto nameToken = matchRegex(word, nameRegex, Token::Type::NAME);
    if (nameToken.has_value()) {
        return nameToken.value();
    }

    return {};
}

std::optional<Token> TextLexer::getToken() {
    std::string_view previousWord = currentWord;
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
        std::cerr << "Found an invalid token: '" << currentWord << "'" << std::endl;
        return std::optional(Token(Token::Type::INVALID, currentWord));
    }

    return {};
}

std::string_view TextLexer::advanceStream(size_t characters) {
    auto tmp    = currentWord.substr(0, characters);
    currentWord = currentWord.substr(characters);
    return tmp;
}

} // namespace pdf
