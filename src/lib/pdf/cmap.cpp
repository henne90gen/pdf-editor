#include "cmap.h"

namespace pdf {

void CMapParser::ignoreNewLineTokens() {
    while (currentTokenIs(Token::Type::NEW_LINE)) {
        currentTokenIdx++;
    }
}

bool CMapParser::ensureTokensHaveBeenLexed() {
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

bool CMapParser::currentTokenIs(Token::Type type) {
    if (!ensureTokensHaveBeenLexed()) {
        return false;
    }

    return tokens[currentTokenIdx].type == type;
}

void CMapParser::parseCodeSpaceRange() {
    auto beforeTokenIdx = currentTokenIdx;
    if (!currentTokenIs(Token::Type::INTEGER)) {
        return;
    }
    auto &integerToken = tokens[currentTokenIdx];

    currentTokenIdx++;
    if (!currentTokenIs(Token::Type::CMAP_BEGIN_CODE_SPACE_RANGE)) {
        currentTokenIdx = beforeTokenIdx;
        return;
    }

    currentTokenIdx++;

    ignoreNewLineTokens();

    std::string str = std::string(integerToken.content);
    auto numRanges  = std::strtol(str.c_str(), nullptr, 10);
    for (int i = 0; i < numRanges; i++) {
        if (!currentTokenIs(Token::Type::HEXADECIMAL_STRING)) {
            currentTokenIdx = beforeTokenIdx;
            return;
        }

//        auto &rangeStart = tokens[currentTokenIdx];

        currentTokenIdx++;
        if (!currentTokenIs(Token::Type::HEXADECIMAL_STRING)) {
            currentTokenIdx = beforeTokenIdx;
            return;
        }

//        auto &rangeEnd = tokens[currentTokenIdx];

        // FIXME save range start and range end somehow

        currentTokenIdx++;
        ignoreNewLineTokens();
    }

    if (!currentTokenIs(Token::Type::CMAP_END_CODE_SPACE_RANGE)) {
        currentTokenIdx = beforeTokenIdx;
        return;
    }

    currentTokenIdx++;
}

void CMapParser::parseBfChar(std::unordered_map<uint8_t, std::string> &charmap) {
    auto beforeTokenIdx = currentTokenIdx;
    if (!currentTokenIs(Token::Type::INTEGER)) {
        return;
    }
    auto &integerToken = tokens[currentTokenIdx];

    currentTokenIdx++;
    if (!currentTokenIs(Token::Type::CMAP_BEGIN_BF_CHAR)) {
        currentTokenIdx = beforeTokenIdx;
        return;
    }

    currentTokenIdx++;

    ignoreNewLineTokens();

    std::string str = std::string(integerToken.content);
    auto numRanges  = std::strtol(str.c_str(), nullptr, 10);
    for (int i = 0; i < numRanges; i++) {
        if (!currentTokenIs(Token::Type::HEXADECIMAL_STRING)) {
            currentTokenIdx = beforeTokenIdx;
            return;
        }

        auto &srcCode = tokens[currentTokenIdx];

        currentTokenIdx++;
        if (!currentTokenIs(Token::Type::HEXADECIMAL_STRING)) {
            currentTokenIdx = beforeTokenIdx;
            return;
        }

        auto &dstCode   = tokens[currentTokenIdx];
        auto srcCodeStr = HexadecimalString(srcCode.content).to_string();
        auto dstCodeStr = HexadecimalString(dstCode.content).to_string();
        while (dstCodeStr[0] == '\0') {
            dstCodeStr = dstCodeStr.substr(1);
        }
        charmap[srcCodeStr[0]] = dstCodeStr;

        currentTokenIdx++;
        ignoreNewLineTokens();
    }

    if (!currentTokenIs(Token::Type::CMAP_END_BF_CHAR)) {
        currentTokenIdx = beforeTokenIdx;
        return;
    }

    currentTokenIdx++;
}

void CMapParser::parseBfRange(std::unordered_map<uint8_t, std::string> &charmap) {
    auto beforeTokenIdx = currentTokenIdx;
    if (!currentTokenIs(Token::Type::INTEGER)) {
        return;
    }
    auto &integerToken = tokens[currentTokenIdx];

    currentTokenIdx++;
    if (!currentTokenIs(Token::Type::CMAP_BEGIN_BF_RANGE)) {
        currentTokenIdx = beforeTokenIdx;
        return;
    }

    currentTokenIdx++;

    ignoreNewLineTokens();

    std::string str = std::string(integerToken.content);
    auto numRanges  = std::strtol(str.c_str(), nullptr, 10);
    for (int i = 0; i < numRanges; i++) {
        // srcCodeLo srcCodeHi dstCodeLo
        // srcCodeLo srcCodeHi [dstString_1 … dstString_M]
        if (!currentTokenIs(Token::Type::HEXADECIMAL_STRING)) {
            currentTokenIdx = beforeTokenIdx;
            return;
        }

        auto &srcCodeLo = tokens[currentTokenIdx];

        currentTokenIdx++;
        if (!currentTokenIs(Token::Type::HEXADECIMAL_STRING)) {
            currentTokenIdx = beforeTokenIdx;
            return;
        }

        auto &srcCodeHi = tokens[currentTokenIdx];
        // FIXME use srcCodeHi somehow

        auto srcCodeLoStr = HexadecimalString(srcCodeLo.content).to_string();
        auto srcCodeHiStr = HexadecimalString(srcCodeHi.content).to_string();

        currentTokenIdx++;
        if (currentTokenIs(Token::Type::HEXADECIMAL_STRING)) {
//            auto &dstCode = tokens[currentTokenIdx];
            // FIXME use dstCode somehow

        } else if (currentTokenIs(Token::Type::ARRAY_START)) {
            currentTokenIdx++;

            uint8_t code = srcCodeLoStr.at(srcCodeLoStr.size() - 1);
            while (currentTokenIs(Token::Type::HEXADECIMAL_STRING)) {
                auto &dstCode   = tokens[currentTokenIdx];
                auto dstCodeStr = HexadecimalString(dstCode.content).to_string();
                charmap[code]   = dstCodeStr;

                currentTokenIdx++;
                code++;
            }

            if (!currentTokenIs(Token::Type::ARRAY_END)) {
                currentTokenIdx = beforeTokenIdx;
                return;
            }
        } else {
            currentTokenIdx = beforeTokenIdx;
            return;
        }

        currentTokenIdx++;
        ignoreNewLineTokens();
    }

    if (!currentTokenIs(Token::Type::CMAP_END_BF_RANGE)) {
        currentTokenIdx = beforeTokenIdx;
        return;
    }

    currentTokenIdx++;
}

CMap *CMapParser::parse() {
    // eat everything before the first 'begincmap'
    while (!currentTokenIs(Token::Type::CMAP_BEGIN)) {
        currentTokenIdx++;
    }

    std::unordered_map<uint8_t, std::string> charmap = {};

    currentTokenIdx++;
    while (true) {
        auto beforeTokenCount = tokens.size();

        parseCodeSpaceRange();

        parseBfChar(charmap);

        parseBfRange(charmap);

        if (currentTokenIs(Token::Type::CMAP_END)) {
            // TODO this is the only exit for this loop, add another exit condition for 'safety'
            break;
        }

        currentTokenIdx++;
        ignoreNewLineTokens();
        if (beforeTokenCount == tokens.size()) {
            break;
        }
    }

    return new CMap(charmap);
}

std::optional<CMap *> CMapStream::read_cmap() {
    auto data         = to_string();
    auto textProvider = StringTextProvider(data);
    auto lexer        = TextLexer(textProvider);
    auto parser       = CMapParser(lexer);

    auto result = parser.parse();
    if (result == nullptr) {
        return {};
    }

    return result;
}

} // namespace pdf
