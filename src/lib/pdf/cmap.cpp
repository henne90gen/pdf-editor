#include "cmap.h"

namespace pdf {

std::optional<std::string> CMap::map_char_code(uint8_t code) const {
    auto itr = charmap.find(code);
    if (itr == charmap.end()) {
        return {};
    }
    return itr->second;
}

std::string CMap::map_char_codes(HexadecimalString *str) const {
    std::string result;
    for (char code : str->to_string()) {
        auto strOpt = map_char_code(code);
        if (strOpt.has_value()) {
            result += strOpt.value();
        }
    }
    return result;
}

void CMapParser::ignore_new_line_tokens() {
    while (current_token_is(Token::Type::NEW_LINE)) {
        currentTokenIdx++;
    }
}

bool CMapParser::ensure_tokens_have_been_lexed() {
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

bool CMapParser::current_token_is(Token::Type type) {
    if (!ensure_tokens_have_been_lexed()) {
        return false;
    }

    return tokens[currentTokenIdx].type == type;
}

void CMapParser::parse_code_space_range() {
    auto beforeTokenIdx = currentTokenIdx;
    if (!current_token_is(Token::Type::INTEGER)) {
        return;
    }

    auto integerToken = tokens[currentTokenIdx];

    currentTokenIdx++;
    if (!current_token_is(Token::Type::CMAP_BEGIN_CODE_SPACE_RANGE)) {
        currentTokenIdx = beforeTokenIdx;
        return;
    }

    currentTokenIdx++;

    ignore_new_line_tokens();

    std::string str = std::string(integerToken.content);
    auto numRanges  = std::strtol(str.c_str(), nullptr, 10);
    for (int i = 0; i < numRanges; i++) {
        if (!current_token_is(Token::Type::HEXADECIMAL_STRING)) {
            currentTokenIdx = beforeTokenIdx;
            return;
        }

        //        auto rangeStart = tokens[currentTokenIdx];

        currentTokenIdx++;
        if (!current_token_is(Token::Type::HEXADECIMAL_STRING)) {
            currentTokenIdx = beforeTokenIdx;
            return;
        }

        //        auto rangeEnd = tokens[currentTokenIdx];

        // FIXME save range start and range end somehow

        currentTokenIdx++;
        ignore_new_line_tokens();
    }

    if (!current_token_is(Token::Type::CMAP_END_CODE_SPACE_RANGE)) {
        currentTokenIdx = beforeTokenIdx;
        return;
    }

    currentTokenIdx++;
}

void CMapParser::parse_bf_char(std::unordered_map<uint8_t, std::string> &charmap) {
    auto beforeTokenIdx = currentTokenIdx;
    if (!current_token_is(Token::Type::INTEGER)) {
        return;
    }

    auto integerToken = tokens[currentTokenIdx];

    currentTokenIdx++;
    if (!current_token_is(Token::Type::CMAP_BEGIN_BF_CHAR)) {
        currentTokenIdx = beforeTokenIdx;
        return;
    }

    currentTokenIdx++;

    ignore_new_line_tokens();

    std::string str = std::string(integerToken.content);
    auto numRanges  = std::strtol(str.c_str(), nullptr, 10);
    for (int i = 0; i < numRanges; i++) {
        if (!current_token_is(Token::Type::HEXADECIMAL_STRING)) {
            currentTokenIdx = beforeTokenIdx;
            return;
        }

        auto srcCode = tokens[currentTokenIdx];

        currentTokenIdx++;
        if (!current_token_is(Token::Type::HEXADECIMAL_STRING)) {
            currentTokenIdx = beforeTokenIdx;
            return;
        }

        auto dstCode    = tokens[currentTokenIdx];
        // FIXME remove '<' and '>' before creating the HexadecimalString
        auto srcCodeStr = HexadecimalString(std::string(srcCode.content)).to_string();
        auto dstCodeStr = HexadecimalString(std::string(dstCode.content)).to_string();
        while (dstCodeStr[0] == '\0') {
            dstCodeStr = dstCodeStr.substr(1);
        }
        charmap[srcCodeStr[0]] = dstCodeStr;

        currentTokenIdx++;
        ignore_new_line_tokens();
    }

    if (!current_token_is(Token::Type::CMAP_END_BF_CHAR)) {
        currentTokenIdx = beforeTokenIdx;
        return;
    }

    currentTokenIdx++;
}

void CMapParser::parse_bf_range(std::unordered_map<uint8_t, std::string> &charmap) {
    auto beforeTokenIdx = currentTokenIdx;
    if (!current_token_is(Token::Type::INTEGER)) {
        return;
    }

    auto integerToken = tokens[currentTokenIdx];

    currentTokenIdx++;
    if (!current_token_is(Token::Type::CMAP_BEGIN_BF_RANGE)) {
        currentTokenIdx = beforeTokenIdx;
        return;
    }

    currentTokenIdx++;

    ignore_new_line_tokens();

    std::string str = std::string(integerToken.content);
    auto numRanges  = std::strtol(str.c_str(), nullptr, 10);
    for (int i = 0; i < numRanges; i++) {
        // srcCodeLo srcCodeHi dstCodeLo
        // srcCodeLo srcCodeHi [dstString_1 … dstString_M]
        if (!current_token_is(Token::Type::HEXADECIMAL_STRING)) {
            currentTokenIdx = beforeTokenIdx;
            return;
        }

        auto srcCodeLo = tokens[currentTokenIdx];

        currentTokenIdx++;
        if (!current_token_is(Token::Type::HEXADECIMAL_STRING)) {
            currentTokenIdx = beforeTokenIdx;
            return;
        }

        auto srcCodeHi = tokens[currentTokenIdx];
        // FIXME use srcCodeHi somehow

        // FIXME remove '<' and '>' before creating the HexadecimalString
        auto srcCodeLoStr = HexadecimalString(std::string(srcCodeLo.content)).to_string();
        auto srcCodeHiStr = HexadecimalString(std::string(srcCodeHi.content)).to_string();

        currentTokenIdx++;
        if (current_token_is(Token::Type::HEXADECIMAL_STRING)) {
            //            auto dstCode = tokens[currentTokenIdx];
            // FIXME use dstCode somehow

        } else if (current_token_is(Token::Type::ARRAY_START)) {
            currentTokenIdx++;

            uint8_t code = srcCodeLoStr.at(srcCodeLoStr.size() - 1);
            while (current_token_is(Token::Type::HEXADECIMAL_STRING)) {
                auto dstCode    = tokens[currentTokenIdx];
                // FIXME remove '<' and '>' before creating the HexadecimalString
                auto dstCodeStr = HexadecimalString(std::string(dstCode.content)).to_string();
                charmap[code]   = dstCodeStr;

                currentTokenIdx++;
                code++;
            }

            if (!current_token_is(Token::Type::ARRAY_END)) {
                currentTokenIdx = beforeTokenIdx;
                return;
            }
        } else {
            currentTokenIdx = beforeTokenIdx;
            return;
        }

        currentTokenIdx++;
        ignore_new_line_tokens();
    }

    if (!current_token_is(Token::Type::CMAP_END_BF_RANGE)) {
        currentTokenIdx = beforeTokenIdx;
        return;
    }

    currentTokenIdx++;
}

CMap *CMapParser::parse() {
    // eat everything before the first 'begincmap'
    while (!current_token_is(Token::Type::CMAP_BEGIN)) {
        currentTokenIdx++;
    }

    std::unordered_map<uint8_t, std::string> charmap = {};

    currentTokenIdx++;
    while (true) {
        auto beforeTokenCount = tokens.size();

        parse_code_space_range();

        parse_bf_char(charmap);

        parse_bf_range(charmap);

        if (current_token_is(Token::Type::CMAP_END)) {
            // TODO this is the only exit for this loop, add another exit condition for 'safety'
            break;
        }

        currentTokenIdx++;
        ignore_new_line_tokens();
        if (beforeTokenCount == tokens.size()) {
            break;
        }
    }

    return allocator.allocate<CMap>(charmap);
}

std::optional<CMap *> CMapStream::read_cmap(Allocator &allocator) {
    auto data         = decode(allocator);
    auto textProvider = StringTextProvider(data);
    auto lexer        = TextLexer(textProvider);
    auto parser       = CMapParser(lexer, allocator);

    auto result = parser.parse();
    if (result == nullptr) {
        return {};
    }

    return result;
}

} // namespace pdf
