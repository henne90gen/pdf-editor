#include "pdf_operation_parser.h"

#include <cstring>

#include "pdf_parser.h"
#include "util.h"

namespace pdf {

std::string replace(std::string str, const std::string &from, const std::string &to) {
    size_t start_pos = str.find(from);
    if (start_pos == std::string::npos)
        return str;
    str.replace(start_pos, from.length(), to);
    return str;
}

Operator::Type stringToOperatorType(const std::string &t) {
    // TODO using string replace to get around the 'x' <--> '*' issue, this could be improved
#define CONDITION(Name, Description)                                                                                   \
    if (t == replace(std::string(#Name), "x", "*")) {                                                                  \
        return Operator::Type::Name##_##Description;                                                                   \
    }
    ENUMERATE_OPERATION_TYPES(CONDITION)
#undef CONDITION
    return Operator::Type::UNKNOWN_UNKNOWN;
}

std::ostream &operator<<(std::ostream &os, Operator::Type &type) {
#define __BYTECODE_OP(Name, Description)                                                                               \
    case Operator::Type::Name##_##Description:                                                                         \
        os.write("Operator::Type::" #Name "_" #Description, strlen("Operator::Type::" #Name "_" #Description));        \
        break;

    switch (type) {
        ENUMERATE_OPERATION_TYPES(__BYTECODE_OP)
    default:
        ASSERT(false);
    }
#undef __BYTECODE_OP
    return os;
}

Operator *OperationParser::getOperator() {
    while (true) {
        while (currentTokenIdx >= tokens.size()) {
            auto t = lexer.getToken();
            if (!t.has_value()) {
                return nullptr;
            }
            tokens.push_back(t.value());
        }

        if (tokens[currentTokenIdx].type == Token::Type::OPERATOR) {
            const std::string &content = tokens[currentTokenIdx].content;
            currentTokenIdx++;
            return createOperator(stringToOperatorType(content));
        }

        currentTokenIdx++;
    }
}

Operator *OperationParser::createOperator_w(Operator *result) {
    auto &content                         = tokens[currentTokenIdx - 2].content;
    auto lineWidth                        = std::stod(content);
    result->data.w_SetLineWidth.lineWidth = lineWidth;
    return result;
}

Operator *OperationParser::createOperator_re(Operator *result) {
    for (int i = 0; i < 4; i++) {
        auto &content                           = tokens[currentTokenIdx - 1 - (4 - i)].content;
        auto d                                  = std::stod(content);
        result->data.re_AppendRectangle.rect[i] = d;
    }
    return result;
}

Operator *OperationParser::createOperator_rg(Operator *result) {
    auto &contentR                           = tokens[currentTokenIdx - 4].content;
    auto &contentG                           = tokens[currentTokenIdx - 3].content;
    auto &contentB                           = tokens[currentTokenIdx - 2].content;
    result->data.rg_SetNonStrokingColorRGB.r = std::stod(contentR);
    result->data.rg_SetNonStrokingColorRGB.g = std::stod(contentG);
    result->data.rg_SetNonStrokingColorRGB.b = std::stod(contentB);
    return result;
}

Operator *OperationParser::createOperator_TJ(Operator *result) {
    int arrayStartIndex = -1;
    for (int i = 0; i < currentTokenIdx; i++) {
        if (tokens[currentTokenIdx - i].type == Token::Type::ARRAY_START) {
            arrayStartIndex = currentTokenIdx - i;
            break;
        }
    }
    ASSERT(arrayStartIndex != -1);

    int objectCount             = currentTokenIdx - 2 - arrayStartIndex;
    const auto first            = tokens.begin() + arrayStartIndex;
    const auto last             = first + objectCount + 1;
    const std::vector<Token> ts = std::vector<Token>(first, last);
    auto l                      = TokenLexer(ts);
    auto p                      = Parser(l);
    auto arr                    = p.parse()->as<Array>();

    ASSERT(arr != nullptr);
    result->data.TJ_ShowOneOrMoreTextStrings.objects = arr;
    return result;
}

Operator *OperationParser::createOperator_Tf(Operator *result) {
    auto &contentFontName = tokens[currentTokenIdx - 3].content;
    ASSERT(contentFontName.size() <= MAX_FONT_NAME_SIZE);
    for (int i = 0; i < contentFontName.size(); i++) {
        result->data.Tf_SetTextFont.fontName[i] = contentFontName[i];
    }
    auto &contentFontSize                = tokens[currentTokenIdx - 2].content;
    result->data.Tf_SetTextFont.fontSize = std::stod(contentFontSize);
    return result;
}

Operator *OperationParser::createOperator_Td(Operator *result) {
    auto &contentX                        = tokens[currentTokenIdx - 3].content;
    auto &contentY                        = tokens[currentTokenIdx - 2].content;
    result->data.Td_MoveStartOfNextLine.x = std::stod(contentX);
    result->data.Td_MoveStartOfNextLine.y = std::stod(contentY);
    return result;
}

Operator *OperationParser::createOperator(Operator::Type type) {
    auto result = new Operator(type);
    if (type == Operator::Type::q_PushGraphicsState || type == Operator::Type::Q_PopGraphicsState ||
        type == Operator::Type::W_ModifyClippingPathUsingNonZeroWindingNumberRule ||
        type == Operator::Type::Wx_ModifyClippingPathUsingEvenOddRule ||
        type == Operator::Type::n_EndPathWithoutFillingOrStroking || type == Operator::Type::BT_BeginText ||
        type == Operator::Type::ET_EndText) {
        // operators without operands
        return result;
    }
    if (type == Operator::Type::w_SetLineWidth) {
        return createOperator_w(result);
    }
    if (type == Operator::Type::re_AppendRectangle) {
        return createOperator_re(result);
    }
    if (type == Operator::Type::rg_SetNonStrokingColorRGB) {
        return createOperator_rg(result);
    }
    if (type == Operator::Type::Td_MoveStartOfNextLine) {
        return createOperator_Td(result);
    }
    if (type == Operator::Type::Tf_SetTextFont) {
        return createOperator_Tf(result);
    }
    if (type == Operator::Type::TJ_ShowOneOrMoreTextStrings) {
        return createOperator_TJ(result);
    }

    std::cerr << "Failed to parse command of type: " << type << "\n";
    ASSERT(false);

    return result;
}

} // namespace pdf
