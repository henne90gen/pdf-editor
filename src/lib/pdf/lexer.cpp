#include "lexer.h"

#include <iostream>
#include <regex>
#include <string>

namespace pdf {

// NOTE pre-computing the regex increased performance 15x
// NOTE moving regex evaluation to the back of findToken increased performance 2x
auto objectStartRegex = std::regex("^[0-9]+ [0-9]+ obj");
auto hexadecimalRegex = std::regex("^<[0-9a-fA-F]*>");
auto nameRegex        = std::regex(R"(^\/[^\r\n\t\f\v /<>\[\]\(\)\{\}]*)");

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
        return {token};
    }
    return {};
}

bool is_digit(char c) {
    for (char d = '0'; d <= '9'; d++) {
        if (c == d) {
            return true;
        }
    }
    return false;
}

std::optional<Token> matchInt(const std::string_view &word) {
    if (word.empty()) {
        return {};
    }

    int idx = 0;
    if (word[idx] == '+' || word[idx] == '-') {
        idx++;
    }

    if (idx >= word.length() || !is_digit(word[idx])) {
        return {};
    }
    idx++;

    while (idx < word.length() && is_digit(word[idx])) {
        idx++;
    }

    return Token(Token::Type::INTEGER, word.substr(0, idx));
}

std::optional<Token> matchFloatOrInt(const std::string_view &word) {
    auto result = matchInt(word);
    if (!result.has_value()) {
        return {};
    }
    auto idx = result.value().content.length();
    if (idx >= word.length() || word[idx] != '.') {
        return result;
    }
    idx++;

    if (idx >= word.length() || !is_digit(word[idx])) {
        return {};
    }
    idx++;

    while (idx < word.length() && is_digit(word[idx])) {
        idx++;
    }

    return Token(Token::Type::REAL, word.substr(0, idx));
}

std::optional<Token> matchWordToken(const std::string_view &word) {
    // TODO combine common prefixes for better performance (begin, end)
#define STARTS_WITH(word, search) word.find(search) == 0
    if (STARTS_WITH(word, "true")) {
        return Token(Token::Type::BOOLEAN, word.substr(0, 4));
    }
    if (STARTS_WITH(word, "false")) {
        return Token(Token::Type::BOOLEAN, word.substr(0, 5));
    }
    if (STARTS_WITH(word, "endobj")) {
        return Token(Token::Type::OBJECT_END, word.substr(0, 6));
    }
    if (STARTS_WITH(word, "stream")) {
        return Token(Token::Type::STREAM_START, word.substr(0, 6));
    }
    if (STARTS_WITH(word, "endstream")) {
        return Token(Token::Type::STREAM_END, word.substr(0, 9));
    }
    if (STARTS_WITH(word, "null")) {
        return Token(Token::Type::NULL_OBJ, word.substr(0, 4));
    }
    if (STARTS_WITH(word, "begincmap")) {
        return Token(Token::Type::CMAP_BEGIN, word.substr(0, 9));
    }
    if (STARTS_WITH(word, "endcmap")) {
        return Token(Token::Type::CMAP_END, word.substr(0, 7));
    }
    if (STARTS_WITH(word, "usecmap")) {
        return Token(Token::Type::CMAP_USE, word.substr(0, 7));
    }
    if (STARTS_WITH(word, "begincodespacerange")) {
        return Token(Token::Type::CMAP_BEGIN_CODE_SPACE_RANGE, word.substr(0, 19));
    }
    if (STARTS_WITH(word, "endcodespacerange")) {
        return Token(Token::Type::CMAP_END_CODE_SPACE_RANGE, word.substr(0, 17));
    }
    if (STARTS_WITH(word, "usefont")) {
        return Token(Token::Type::CMAP_USE_FONT, word.substr(0, 7));
    }
    if (STARTS_WITH(word, "beginbfchar")) {
        return Token(Token::Type::CMAP_BEGIN_BF_CHAR, word.substr(0, 11));
    }
    if (STARTS_WITH(word, "endbfchar")) {
        return Token(Token::Type::CMAP_END_BF_CHAR, word.substr(0, 9));
    }
    if (STARTS_WITH(word, "beginbfrange")) {
        return Token(Token::Type::CMAP_BEGIN_BF_RANGE, word.substr(0, 12));
    }
    if (STARTS_WITH(word, "endbfrange")) {
        return Token(Token::Type::CMAP_END_BF_RANGE, word.substr(0, 10));
    }
    if (STARTS_WITH(word, "begincidchar")) {
        return Token(Token::Type::CMAP_BEGIN_CID_CHAR, word.substr(0, 12));
    }
    if (STARTS_WITH(word, "endcidchar")) {
        return Token(Token::Type::CMAP_END_CID_CHAR, word.substr(0, 10));
    }
    if (STARTS_WITH(word, "begincidrange")) {
        return Token(Token::Type::CMAP_BEGIN_CID_RANGE, word.substr(0, 13));
    }
    if (STARTS_WITH(word, "endcidrange")) {
        return Token(Token::Type::CMAP_END_CID_RANGE, word.substr(0, 11));
    }
    if (STARTS_WITH(word, "beginnotdefchar")) {
        return Token(Token::Type::CMAP_BEGIN_NOTDEF_CHAR, word.substr(0, 15));
    }
    if (STARTS_WITH(word, "endnotdefchar")) {
        return Token(Token::Type::CMAP_END_NOTDEF_CHAR, word.substr(0, 13));
    }
    if (STARTS_WITH(word, "beginnotdefrange")) {
        return Token(Token::Type::CMAP_BEGIN_NOTDEF_RANGE, word.substr(0, 16));
    }
    if (STARTS_WITH(word, "endnotdefrange")) {
        return Token(Token::Type::CMAP_END_NOTDEF_RANGE, word.substr(0, 14));
    }
    return {};
}

std::optional<Token> matchCharToken(const std::string_view &word) {
    if (!word.empty() && word[0] == '\n') {
        return Token(Token::Type::NEW_LINE, word.substr(0, 1));
    }
    if (STARTS_WITH(word, "\r\n")) {
        return Token(Token::Type::NEW_LINE, word.substr(0, 2));
    }
    if (!word.empty() && word[0] == '[') {
        return Token(Token::Type::ARRAY_START, word.substr(0, 1));
    }
    if (!word.empty() && word[0] == ']') {
        return Token(Token::Type::ARRAY_END, word.substr(0, 1));
    }
    if (STARTS_WITH(word, "<<")) {
        return Token(Token::Type::DICTIONARY_START, word.substr(0, 2));
    }
    if (STARTS_WITH(word, ">>")) {
        return Token(Token::Type::DICTIONARY_END, word.substr(0, 2));
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

std::optional<Token> matchIndirectReference(const std::string_view &word) {
    auto num1 = matchInt(word);
    if (!num1.has_value()) {
        return {};
    }

    auto idx = num1.value().content.length();
    if (idx >= word.length() || word[idx] != ' ') {
        return {};
    }
    idx++;

    auto num2 = matchInt(word.substr(idx));
    if (!num2.has_value()) {
        return {};
    }

    idx += num2.value().content.length();
    if (idx >= word.length() || word[idx] != ' ') {
        return {};
    }
    idx++;

    if (idx >= word.length() || word[idx] != 'R') {
        return {};
    }
    idx++;

    return Token(Token::Type::INDIRECT_REFERENCE, word.substr(0, idx));
}

std::optional<Token> findToken(const std::string_view &word) {
    auto literalString = matchString(word);
    if (literalString.has_value()) {
        return literalString;
    }

    auto charToken = matchCharToken(word);
    if (charToken.has_value()) {
        return charToken;
    }

    auto wordToken = matchWordToken(word);
    if (wordToken.has_value()) {
        return wordToken;
    }

    auto operatorToken = matchOperator(word);
    if (operatorToken.has_value()) {
        return operatorToken;
    }

    // NOTE indirect reference and object start have to be lexed before float or int
    auto indirectReferenceToken = matchIndirectReference(word);
    if (indirectReferenceToken.has_value()) {
        return indirectReferenceToken;
    }

    auto objectStartToken = matchRegex(word, objectStartRegex, Token::Type::OBJECT_START);
    if (objectStartToken.has_value()) {
        return objectStartToken;
    }

    auto floatOrIntToken = matchFloatOrInt(word);
    if (floatOrIntToken.has_value()) {
        return floatOrIntToken;
    }

    auto hexadecimalString = matchRegex(word, hexadecimalRegex, Token::Type::HEXADECIMAL_STRING);
    if (hexadecimalString.has_value()) {
        return hexadecimalString;
    }

    // TODO check with the standard again
    auto nameToken = matchRegex(word, nameRegex, Token::Type::NAME);
    if (nameToken.has_value()) {
        return nameToken;
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
        return Token(Token::Type::INVALID, currentWord);
    }

    return {};
}

std::string_view TextLexer::advanceStream(size_t characters) {
    auto tmp    = currentWord.substr(0, characters);
    currentWord = currentWord.substr(characters);
    return tmp;
}

} // namespace pdf
