#include "operator_parser.h"

#include <cstring>

#include "parser.h"
#include "util.h"

namespace pdf {

std::string replace(std::string str, const std::string &from, const std::string &to) {
    size_t start_pos = str.find(from);
    if (start_pos == std::string::npos)
        return str;
    str.replace(start_pos, from.length(), to);
    return str;
}

Operator::Type stringToOperatorType(const std::string_view &t) {
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

Operator *OperatorParser::getOperator() {
    while (true) {
        while (currentTokenIdx >= tokens.size()) {
            auto t = lexer.getToken();
            if (!t.has_value()) {
                return nullptr;
            }
            tokens.push_back(t.value());
        }

        if (tokens[currentTokenIdx].type == Token::Type::OPERATOR) {
            const std::string_view &content = tokens[currentTokenIdx].content;
            currentTokenIdx++;
            return createOperator(stringToOperatorType(content));
        }

        currentTokenIdx++;
    }
}

Operator *OperatorParser::createOperator_w(Operator *result) {
    result->data.w_SetLineWidth.lineWidth = operand<double>(0);
    return result;
}

Operator *OperatorParser::createOperator_re(Operator *result) {
    for (int i = 0; i < 4; i++) {
        auto &content = tokens[currentTokenIdx - 1 - (4 - i)].content;
        // TODO is this conversion to a string really necessary?
        // TODO catch exception
        result->data.re_AppendRectangle.rect[i] = std::stod(std::string(content));
    }
    return result;
}

Operator *OperatorParser::createOperator_rg(Operator *result) {
    result->data.rg_SetNonStrokingColorRGB.r = operand<double>(2);
    result->data.rg_SetNonStrokingColorRGB.g = operand<double>(1);
    result->data.rg_SetNonStrokingColorRGB.b = operand<double>(0);
    return result;
}

Operator *OperatorParser::createOperator_TJ(Operator *result) {
    int arrayStartIndex = -1;
    for (int i = 0; i < currentTokenIdx; i++) {
        if (tokens[currentTokenIdx - i].type == Token::Type::ARRAY_START) {
            arrayStartIndex = currentTokenIdx - i;
            break;
        }
    }
    ASSERT(arrayStartIndex != -1);

    int objectCount  = currentTokenIdx - 2 - arrayStartIndex;
    const auto first = tokens.begin() + arrayStartIndex;
    const auto last  = first + objectCount + 1;
    const auto ts    = std::vector<Token>(first, last);
    auto l           = TokenLexer(ts);
    auto p           = Parser(l);
    auto arr         = p.parse()->as<Array>();

    ASSERT(arr != nullptr);
    result->data.TJ_ShowOneOrMoreTextStrings.objects = arr;
    return result;
}

Operator *OperatorParser::createOperator_Tf(Operator *result) {
    auto contentFontName                              = tokens[currentTokenIdx - 3].content;
    result->data.Tf_SetTextFontAndSize.fontNameData   = contentFontName.data();
    result->data.Tf_SetTextFontAndSize.fontNameLength = contentFontName.length();
    result->data.Tf_SetTextFontAndSize.fontSize       = operand<double>(0);
    return result;
}

Operator *OperatorParser::createOperator_Td(Operator *result) {
    result->data.Td_MoveStartOfNextLine.x = operand<double>(1);
    result->data.Td_MoveStartOfNextLine.y = operand<double>(0);
    return result;
}

Operator *OperatorParser::createOperator_J(Operator *result) {
    result->data.J_LineCapStyle.lineCap = operand<int64_t>(0);
    return result;
}

Operator *OperatorParser::createOperator_Tm(Operator *result) {
    // a b c d e f Tm
    for (int i = 0; i < 6; i++) {
        result->data.Tm_SetTextMatrixAndTextLineMatrix.matrix[i] = operand<double>(5 - i);
    }
    return result;
}

Operator *OperatorParser::createOperator_Tj(Operator *result) {
    result->data.Tj_ShowTextString.string = new LiteralString(tokens[currentTokenIdx - 2].content);
    return result;
}

Operator *OperatorParser::createOperator(Operator::Type type) {
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
    if (type == Operator::Type::Tf_SetTextFontAndSize) {
        return createOperator_Tf(result);
    }
    if (type == Operator::Type::Tj_ShowTextString) {
        return createOperator_Tj(result);
    }
    if (type == Operator::Type::TJ_ShowOneOrMoreTextStrings) {
        return createOperator_TJ(result);
    }
    if (type == Operator::Type::J_SetLineCapStyle) {
        return createOperator_J(result);
    }
    if (type == Operator::Type::Tm_SetTextMatrixAndTextLineMatrix) {
        return createOperator_Tm(result);
    }

    std::cerr << "Failed to parse command of type: " << type << "\n";
    ASSERT(false);

    return result;
}

} // namespace pdf
