#include "cmap.h"

namespace pdf {

bool CMapParser::currentTokenIs(Token::Type type) {
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

CMap *CMapParser::parse() {
    if (!currentTokenIs(Token::Type::CMAP_BEGIN)) {
        return nullptr;
    }

    while (true) {
        auto beforeTokenCount = tokens.size();
        if (currentTokenIs(Token::Type::CMAP_BEGIN_CODE_SPACE_RANGE)) {
            // parse codespacerange
        }
        if (currentTokenIs(Token::Type::CMAP_BEGIN_BF_CHAR)) {
            // parse bfchar
        }
        currentTokenIdx++;
        if (beforeTokenCount == tokens.size()) {
            break;
        }
    }

    if (currentTokenIs(Token::Type::CMAP_BEGIN_CODE_SPACE_RANGE)) {
        std::cout << "Hello" << std::endl;
    }

    if (currentTokenIs(Token::Type::CMAP_BEGIN_BF_CHAR)) {
        std::cout << "Hello" << std::endl;
    }

    return nullptr;
}

std::optional<CMap *> CMapStream::read_cmap() {
    auto data = to_string();
    auto idx  = data.find("begincmap");
    if (idx == -1) {
        return {};
    }

    auto textProvider = StringTextProvider(data.substr(idx));
    auto lexer        = TextLexer(textProvider);
    auto parser       = CMapParser(lexer);

    auto result = parser.parse();
    if (result == nullptr) {
        return {};
    }

    return result;
}

} // namespace pdf
