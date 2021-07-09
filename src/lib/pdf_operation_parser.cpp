#include "pdf_operation_parser.h"

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

Operator *OperationParser::createOperator(Operator::Type type) {
    auto result = new Operator(type);
    if (type == Operator::Type::w_SetLineWidth) {
        auto &content                         = tokens[currentTokenIdx - 2].content;
        auto lineWidth                        = std::stod(content);
        result->data.w_SetLineWidth.lineWidth = lineWidth;
        return result;
    }
    if (type == Operator::Type::re_AppendRectangle) {
        for (int i = 0; i < 4; i++) {
            auto &content                           = tokens[currentTokenIdx - 1 - (4 - i)].content;
            auto d                                  = std::stod(content);
            result->data.re_AppendRectangle.rect[i] = d;
        }
        return result;
    }
    if (type == Operator::Type::rg_SetNonStrokingColorRGB) {
        auto &contentR                           = tokens[currentTokenIdx - 4].content;
        auto &contentG                           = tokens[currentTokenIdx - 3].content;
        auto &contentB                           = tokens[currentTokenIdx - 2].content;
        result->data.rg_SetNonStrokingColorRGB.r = std::stod(contentR);
        result->data.rg_SetNonStrokingColorRGB.g = std::stod(contentG);
        result->data.rg_SetNonStrokingColorRGB.b = std::stod(contentB);
        return result;
    }
    if (type == Operator::Type::Td_MoveStartOfNextLine) {
        auto &contentX                        = tokens[currentTokenIdx - 3].content;
        auto &contentY                        = tokens[currentTokenIdx - 2].content;
        result->data.Td_MoveStartOfNextLine.x = std::stod(contentX);
        result->data.Td_MoveStartOfNextLine.y = std::stod(contentY);
        return result;
    }
    if (type == Operator::Type::Tf_SetTextFont) {
        auto &contentFontName = tokens[currentTokenIdx - 3].content;
        ASSERT(contentFontName.size() <= MAX_FONT_NAME_SIZE);
        for (int i = 0; i < contentFontName.size(); i++) {
            result->data.Tf_SetTextFont.fontName[i] = contentFontName[i];
        }
        auto &contentFontSize                = tokens[currentTokenIdx - 2].content;
        result->data.Tf_SetTextFont.fontSize = std::stod(contentFontSize);
        return result;
    }
    if (type == Operator::Type::TJ_ShowOneOrMoreTextStrings) {
        int arrayStartIndex = -1;
        for (int i = 0; i < currentTokenIdx; i++) {
            if (tokens[currentTokenIdx - i].type == Token::Type::ARRAY_START) {
                arrayStartIndex = currentTokenIdx - i + 1;
                break;
            }
        }
        ASSERT(arrayStartIndex != -1);
        int objectCount = currentTokenIdx - 2 - arrayStartIndex;
        //      [<01>-2<02>1<03>2<03>2<0405>17<06>76<040708>]
//        result->data.TJ_ShowOneOrMoreTextStrings.array = (Object **)malloc(objectCount * sizeof(Object *));
//        for (int i = 0; i < objectCount;i++) {
//            result->data.TJ_ShowOneOrMoreTextStrings.array[i]=
//        }
        return result;
    }
    return result;
}

} // namespace pdf
