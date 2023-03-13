#include "lexer.h"

#include <array>
#include <iostream>
#include <spdlog/spdlog.h>
#include <string>

namespace pdf {

static std::array<std::string, 59> operators = {
      // Unsorted Operators
      "SCN", "SC", "scn", "sc",
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
      "Tj", "TJ", "d0", "d1", "CS", "G", "g", "RG", "rg", "K", "k",
      "Do", //
};

bool is_lower_letter(char c) { return c >= 'a' && c <= 'z'; }
bool is_upper_letter(char c) { return c >= 'A' && c <= 'Z'; }
bool is_letter(char c) { return is_lower_letter(c) || is_upper_letter(c); }
bool is_digit(char c) { return c >= '0' && c <= '9'; }

std::string_view removeLeadingWhitespace(const std::string_view &str) {
    std::string_view result = str;
    while (!result.empty() && (result[0] == ' ' || result[0] == '\t')) {
        result = result.substr(1, result.length());
    }
    return result;
}

std::optional<Token> matchInt(const std::string_view &word) {
    if (word.empty()) {
        return {};
    }

    size_t idx = 0;
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
    if (STARTS_WITH(word, "findresource")) {
        return Token(Token::Type::FIND_RESOURCE, word.substr(0, 12));
    }
    if (STARTS_WITH(word, "defineresource")) {
        return Token(Token::Type::DEFINE_RESOURCE, word.substr(0, 14));
    }
    if (STARTS_WITH(word, "def")) {
        return Token(Token::Type::DEF, word.substr(0, 3));
    }
    if (STARTS_WITH(word, "dict")) {
        return Token(Token::Type::DICT, word.substr(0, 4));
    }
    if (STARTS_WITH(word, "dup")) {
        return Token(Token::Type::DUP, word.substr(0, 3));
    }
    if (STARTS_WITH(word, "pop")) {
        return Token(Token::Type::POP, word.substr(0, 3));
    }
    if (STARTS_WITH(word, "currentdict")) {
        return Token(Token::Type::CURRENT_DICT, word.substr(0, 11));
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
    if (STARTS_WITH(word, "begin")) {
        return Token(Token::Type::BEGIN, word.substr(0, 5));
    }
    if (STARTS_WITH(word, "end")) {
        return Token(Token::Type::END, word.substr(0, 3));
    }
    if (STARTS_WITH(word, "CMapName")) {
        return Token(Token::Type::CMAP_NAME, word.substr(0, 8));
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
    if (STARTS_WITH(word, '\r')) {
        return Token(Token::Type::NEW_LINE, word.substr(0, 1));
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

    int openParenthesis = 1;
    int stringLength    = -1;
    for (int i = 1; i < static_cast<int>(word.size()); i++) {
        if (word[i] == '(' && word[i - 1] != '\\') {
            openParenthesis++;
        } else if (word[i] == ')' && word[i - 1] != '\\') {
            openParenthesis--;
        }
        if (openParenthesis == 0) {
            stringLength = i;
            break;
        }
    }

    if (openParenthesis != 0) {
        return {};
    }

    return Token(Token::Type::LITERAL_STRING, word.substr(0, stringLength + 1));
}

std::optional<Token> matchOperator(const std::string_view &word) {
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

std::optional<Token> matchObjectStart(const std::string_view &word) {
    int idx = 0;
    while (is_digit(word[idx])) {
        idx++;
    }

    if (word[idx] != ' ') {
        return {};
    }
    idx++;

    while (is_digit(word[idx])) {
        idx++;
    }

    if (word[idx] != ' ') {
        return {};
    }
    idx++;

    if (word.substr(idx, 3) != "obj") {
        return {};
    }
    idx += 3;

    return Token(Token::Type::OBJECT_START, word.substr(0, idx));
}

std::optional<Token> matchHexadecimalString(const std::string_view &word) {
    int idx = 0;
    if (word[idx] != '<') {
        return {};
    }
    idx++;

    while (word[idx] != '>') {
        if (!is_letter(word[idx])   //
            && !is_digit(word[idx]) //
            && word[idx] != ' '     //
            && word[idx] != '\r'    //
            && word[idx] != '\n'    //
            && word[idx] != '\t'    //
            && word[idx] != '\f'    //
            && word[idx] != '\000') {
            return {};
        }
        idx++;
    }
    idx++;

    return Token(Token::Type::HEXADECIMAL_STRING, word.substr(0, idx));
}

std::optional<Token> matchName(const std::string_view &word) {
    int idx = 0;
    if (word[idx] != '/') {
        return {};
    }
    idx++;

    while (word[idx] != ' ' && word[idx] != '[' && word[idx] != ']' && word[idx] != '(' && word[idx] != ')' &&
           word[idx] != '{' && word[idx] != '}' && word[idx] != '/' && word[idx] != '<' && word[idx] != '>' &&
           word[idx] != '\r' && word[idx] != '\n' && word[idx] != '\t' && word[idx] != '\f' && word[idx] != '\v') {
        idx++;
    }

    return Token(Token::Type::NAME, word.substr(0, idx));
}

std::optional<Token> matchComment(const std::string_view &word) {
    if (word[0] != '%') {
        return {};
    }

    size_t idx = 1;
    while (idx < word.length() && word[idx] != '\n' && word[idx] != '\r') {
        idx++;
    }

    if (idx >= word.length()) {
        return {};
    }

    return Token(Token::Type::COMMENT, word.substr(0, idx));
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

    // NOTE indirect reference and object start have to be lexed before float or int
    auto indirectReferenceToken = matchIndirectReference(word);
    if (indirectReferenceToken.has_value()) {
        return indirectReferenceToken;
    }

    auto objectStartToken = matchObjectStart(word);
    if (objectStartToken.has_value()) {
        return objectStartToken;
    }

    auto floatOrIntToken = matchFloatOrInt(word);
    if (floatOrIntToken.has_value()) {
        return floatOrIntToken;
    }

    auto hexadecimalStringToken = matchHexadecimalString(word);
    if (hexadecimalStringToken.has_value()) {
        return hexadecimalStringToken;
    }

    auto nameToken = matchName(word);
    if (nameToken.has_value()) {
        return nameToken;
    }

    auto commentToken = matchComment(word);
    if (commentToken.has_value()) {
        return commentToken;
    }

    auto operatorToken = matchOperator(word);
    if (operatorToken.has_value()) {
        return operatorToken;
    }

    return {};
}

std::optional<Token> TextLexer::get_token() {
    std::string_view previousWord = currentWord;
    while (true) {
        if (currentWord.empty()) {
            auto optionalCode = textProvider.get_text();
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
        return Token(Token::Type::INVALID, currentWord);
    }

    return {};
}

std::string_view TextLexer::advance_stream(size_t characters) {
    if (currentWord.length() <= characters) {
        // TODO fetch more text from the textProvider
        return {};
    }

    auto tmp    = currentWord.substr(0, characters);
    currentWord = currentWord.substr(characters);
    return tmp;
}

} // namespace pdf
