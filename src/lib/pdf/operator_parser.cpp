#include "operator_parser.h"

#include <cstring>
#include <spdlog/spdlog.h>

#include "parser.h"

namespace pdf {

template <> double OperatorParser::operand(int index) {
    Token &token = tokens[currentTokenIdx - (2 + index)];
    if (token.type == Token::Type::NEW_LINE) {
        index++;
        token = tokens[currentTokenIdx - (2 + index)];
    }
    auto &content = token.content;
    // TODO is this conversion to a string really necessary?
    // TODO catch exception
    return std::stod(std::string(content));
}

template <> int64_t OperatorParser::operand(int index) {
    auto &content = tokens[currentTokenIdx - (2 + index)].content;
    // TODO is this conversion to a string really necessary?
    // TODO catch exception
    return std::stoll(std::string(content));
}

template <> std::string_view OperatorParser::operand(int index) {
    return tokens[currentTokenIdx - (2 + index)].content;
}

template <> std::string OperatorParser::operand(int index) {
    return std::string(tokens[currentTokenIdx - (2 + index)].content);
}

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

std::string operatorTypeToString(Operator::Type &type) {
#define __BYTECODE_OP(Name, Description)                                                                               \
    case Operator::Type::Name##_##Description:                                                                         \
        return "Operator::Type::" #Name "_" #Description;                                                              \
        break;

    switch (type) {
        ENUMERATE_OPERATION_TYPES(__BYTECODE_OP)
    default:
        ASSERT(false);
    }
#undef __BYTECODE_OP
}

Operator *OperatorParser::get_operator() {
    while (true) {
        while (currentTokenIdx >= tokens.size()) {
            auto t = lexer.get_token();
            if (!t.has_value()) {
                return nullptr;
            }
            tokens.push_back(t.value());
        }

        if (tokens[currentTokenIdx].type == Token::Type::NEW_LINE) {
            lastOperatorEnd += tokens[currentTokenIdx].content.size();
        }

        if (tokens[currentTokenIdx].type == Token::Type::OPERATOR) {
            if (lastOperatorEnd == nullptr) {
                lastOperatorEnd = tokens[0].content.data();
            }

            const auto operatorContent               = tokens[currentTokenIdx].content;
            const auto operatorContentWithParameters = std::string_view(
                  lastOperatorEnd, (operatorContent.data() - lastOperatorEnd) + operatorContent.size());
            lastOperatorEnd = operatorContent.data() + operatorContent.size();
            if (*lastOperatorEnd == ' ') {
                lastOperatorEnd++;
            }

            currentTokenIdx++;
            return create_operator(stringToOperatorType(operatorContent), operatorContentWithParameters);
        }

        if (tokens[currentTokenIdx].type == Token::Type::INVALID) {
            spdlog::warn("Found an invalid token in the operator stream");
            return nullptr;
        }

        currentTokenIdx++;
    }
}

Operator *OperatorParser::create_operator_w(Operator *result) {
    result->data.w_SetLineWidth.lineWidth = operand<double>(0);
    return result;
}

Operator *OperatorParser::create_operator_re(Operator *result) {
    for (int i = 0; i < 4; i++) {
        auto &content = tokens[currentTokenIdx - 1 - (4 - i)].content;
        // TODO is this conversion to a string really necessary?
        // TODO catch exception
        result->data.re_AppendRectangle.rect[i] = std::stod(std::string(content));
    }
    return result;
}

Operator *OperatorParser::create_operator_rg(Operator *result) {
    result->data.rg_SetNonStrokingColorRGB.r = operand<double>(2);
    result->data.rg_SetNonStrokingColorRGB.g = operand<double>(1);
    result->data.rg_SetNonStrokingColorRGB.b = operand<double>(0);
    return result;
}

Operator *OperatorParser::create_operator_TJ(Operator *result) {
    int arrayStartIndex = -1;
    for (size_t i = 0; i < currentTokenIdx; i++) {
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
    auto p           = Parser(l, allocator);
    auto arr         = p.parse()->as<Array>();

    ASSERT(arr != nullptr);
    result->data.TJ_ShowOneOrMoreTextStrings.objects = arr;
    return result;
}

Operator *OperatorParser::create_operator_Tf(Operator *result) {
    auto contentFontName                              = operand<std::string_view>(1);
    result->data.Tf_SetTextFontAndSize.fontNameData   = contentFontName.data();
    result->data.Tf_SetTextFontAndSize.fontNameLength = contentFontName.length();
    result->data.Tf_SetTextFontAndSize.fontSize       = operand<double>(0);
    return result;
}

Operator *OperatorParser::create_operator_Td(Operator *result) {
    result->data.Td_MoveStartOfNextLine.x = operand<double>(1);
    result->data.Td_MoveStartOfNextLine.y = operand<double>(0);
    return result;
}

Operator *OperatorParser::create_operator_J(Operator *result) {
    result->data.J_LineCapStyle.lineCap = operand<int64_t>(0);
    return result;
}

Operator *OperatorParser::create_operator_Tm(Operator *result) {
    // a b c d e f Tm
    for (int i = 0; i < 6; i++) {
        result->data.Tm_SetTextMatrixAndTextLineMatrix.matrix[i] = operand<double>(5 - i);
    }
    return result;
}

Operator *OperatorParser::create_operator_Tj(Operator *result) {
    result->data.Tj_ShowTextString.string = allocator.allocate<LiteralString>(operand<std::string>(0));
    return result;
}

Operator *OperatorParser::create_operator_cm(Operator *result) {
    // FIXME parse operator 'cm'
    return result;
}

Operator *OperatorParser::create_operator_g(Operator *result) {
    // FIXME parse operator 'g'
    return result;
}

Operator *OperatorParser::create_operator_d(Operator *result) {
    // FIXME parse operator 'd'
    return result;
}

Operator *OperatorParser::create_operator_c(Operator *result) {
    // FIXME parse operator 'c'
    return result;
}

Operator *OperatorParser::create_operator_s(Operator *result) {
    // FIXME parse operator 's'
    return result;
}

Operator *OperatorParser::create_operator_Tc(Operator *result) {
    // FIXME parse operator 'Tc'
    return result;
}

Operator *OperatorParser::create_operator_CS(Operator *result) {
    // FIXME parse operator 'CS'
    return result;
}

Operator *OperatorParser::create_operator_S(Operator *result) {
    // FIXME parse operator 'S'
    return result;
}

Operator *OperatorParser::create_operator_sc(Operator *result) {
    // FIXME parse operator 'sc'
    return result;
}

Operator *OperatorParser::create_operator_SC(Operator *result) {
    // FIXME parse operator 'SC'
    return result;
}

Operator *OperatorParser::create_operator_m(Operator *result) {
    // FIXME parse operator 'm'
    return result;
}

Operator *OperatorParser::create_operator_B(Operator *result) {
    // FIXME parse operator 'B'
    return result;
}

Operator *OperatorParser::create_operator_Tw(Operator *result) {
    // FIXME parse operator 'Tw'
    return result;
}

Operator *OperatorParser::create_operator_G(Operator *result) {
    // FIXME parse operator 'G'
    return result;
}

Operator *OperatorParser::create_operator_Tz(Operator *result) {
    // FIXME parse operator 'Tz'
    return result;
}

Operator *OperatorParser::create_operator_f(Operator *result) {
    // FIXME parse operator 'f'
    return result;
}

Operator *OperatorParser::create_operator_gs(Operator *result) {
    // FIXME parse operator 'gs'
    return result;
}

Operator *OperatorParser::create_operator_Do(Operator *result) {
    auto name                         = operand<std::string>(0);
    result->data.Do_PaintXObject.name = allocator.allocate<Name>(name);
    return result;
}

Operator *OperatorParser::create_operator(Operator::Type type, std::string_view content) {
    auto result = allocator.allocate<Operator>(type, content);
    if (type == Operator::Type::q_PushGraphicsState || type == Operator::Type::Q_PopGraphicsState ||
        type == Operator::Type::W_ModifyClippingPathUsingNonZeroWindingNumberRule ||
        type == Operator::Type::Wx_ModifyClippingPathUsingEvenOddRule ||
        type == Operator::Type::n_EndPathWithoutFillingOrStroking || type == Operator::Type::BT_BeginText ||
        type == Operator::Type::ET_EndText) {
        // operators without operands
        return result;
    }
    if (type == Operator::Type::w_SetLineWidth) {
        return create_operator_w(result);
    }
    if (type == Operator::Type::re_AppendRectangle) {
        return create_operator_re(result);
    }
    if (type == Operator::Type::rg_SetNonStrokingColorRGB) {
        return create_operator_rg(result);
    }
    if (type == Operator::Type::Td_MoveStartOfNextLine) {
        return create_operator_Td(result);
    }
    if (type == Operator::Type::Tf_SetTextFontAndSize) {
        return create_operator_Tf(result);
    }
    if (type == Operator::Type::Tj_ShowTextString) {
        return create_operator_Tj(result);
    }
    if (type == Operator::Type::TJ_ShowOneOrMoreTextStrings) {
        return create_operator_TJ(result);
    }
    if (type == Operator::Type::J_SetLineCapStyle) {
        return create_operator_J(result);
    }
    if (type == Operator::Type::Tm_SetTextMatrixAndTextLineMatrix) {
        return create_operator_Tm(result);
    }
    if (type == Operator::Type::cm_ModifyCurrentTransformationMatrix) {
        return create_operator_cm(result);
    }
    if (type == Operator::Type::g_SetNonStrokingColorGray) {
        return create_operator_g(result);
    }
    if (type == Operator::Type::d_SetLineDashPattern) {
        return create_operator_d(result);
    }
    if (type == Operator::Type::c_AppendCubicBezier) {
        return create_operator_c(result);
    }
    if (type == Operator::Type::s_UNKNOWN) {
        return create_operator_s(result);
    }
    if (type == Operator::Type::Tc_SetCharacterSpacing) {
        return create_operator_Tc(result);
    }
    if (type == Operator::Type::CS_UNKNOWN) {
        return create_operator_CS(result);
    }
    if (type == Operator::Type::S_UNKNOWN) {
        return create_operator_S(result);
    }
    if (type == Operator::Type::sc_UNKNOWN) {
        return create_operator_sc(result);
    }
    if (type == Operator::Type::SC_UNKNOWN) {
        return create_operator_SC(result);
    }
    if (type == Operator::Type::m_AppendNewSubpath) {
        return create_operator_m(result);
    }
    if (type == Operator::Type::B_UNKNOWN) {
        return create_operator_B(result);
    }
    if (type == Operator::Type::Tw_SetWordSpacing) {
        return create_operator_Tw(result);
    }
    if (type == Operator::Type::G_SetStrokingColorGray) {
        return create_operator_G(result);
    }
    if (type == Operator::Type::Tz_SetHorizontalScaling) {
        return create_operator_Tz(result);
    }
    if (type == Operator::Type::f_UNKNOWN) {
        return create_operator_f(result);
    }
    if (type == Operator::Type::gs_SetParametersGraphicsState) {
        return create_operator_gs(result);
    }
    if (type == Operator::Type::Do_PaintXObject) {
        return create_operator_Do(result);
    }

    spdlog::error("Failed to parse command of type: {}", operatorTypeToString(type));
    ASSERT(false);

    return result;
}

} // namespace pdf
