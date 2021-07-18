#include "renderer.h"

#include "operator_parser.h"
#include "util.h"

namespace pdf {

void renderer::render(const Cairo::RefPtr<Cairo::Context> &cr) {
    auto contentsOpt = page->contents();
    if (!contentsOpt.has_value()) {
        return;
    }

    // TODO set graphics state to default values

    auto content = contentsOpt.value();

    if (content->is<Stream>()) {
        render(cr, {content->as<Stream>()});
    } else if (content->is<Array>()) {
        auto arr     = content->as<Array>();
        auto streams = std::vector<Stream *>(arr->values.size());
        for (auto val : arr->values) {
            streams.push_back(val->as<Stream>());
        }
        render(cr, streams);
    } else {
        // NOTE this should never be reached
        ASSERT(false);
    }
}

void renderer::render(const Cairo::RefPtr<Cairo::Context> &cr, const std::vector<Stream *> &streams) {
    for (auto s : streams) {
        auto textProvider   = StringTextProvider(s->to_string());
        auto lexer          = TextLexer(textProvider);
        auto operatorParser = OperatorParser(lexer);
        Operator *op        = operatorParser.getOperator();
        while (op != nullptr) {
            if (op->type == Operator::Type::w_SetLineWidth) {
                stateStack.back().lineWidth = op->data.w_SetLineWidth.lineWidth;
            } else if (op->type == Operator::Type::q_PushGraphicsState) {
                pushGraphicsState();
            } else if (op->type == Operator::Type::Q_PopGraphicsState) {
                popGraphicsState();
            } else if (op->type == Operator::Type::re_AppendRectangle) {
                TODO("append rectangle to the current path");
            } else if (op->type == Operator::Type::Wx_ModifyClippingPathUsingEvenOddRule) {
                TODO("clipping path modification");
            } else if (op->type == Operator::Type::n_EndPathWithoutFillingOrStroking) {
                // TODO this is a path painting no-op
                //  it does however set the clipping path, if a clipping path operator was used before it
                TODO("path painting no-op");
            } else if (op->type == Operator::Type::rg_SetNonStrokingColorRGB) {
                setNonStrokingColor(op);
            } else if (op->type == Operator::Type::BT_BeginText) {
                beginText();
            } else if (op->type == Operator::Type::ET_EndText) {
                endText();
            } else if (op->type == Operator::Type::Td_MoveStartOfNextLine) {
                moveStartOfNextLine(op);
            } else if (op->type == Operator::Type::Tf_SetTextFont) {
                setTextFont(op);
            } else if (op->type == Operator::Type::TJ_ShowOneOrMoreTextStrings) {
                showText(op);
            } else {
                TODO("unknown operator");
            }
            op = operatorParser.getOperator();
        }
    }
}

void renderer::setNonStrokingColor(Operator *op) {
    stateStack.back().nonStrokingColor = Color::rgb( //
          op->data.rg_SetNonStrokingColorRGB.r,      //
          op->data.rg_SetNonStrokingColorRGB.g,      //
          op->data.rg_SetNonStrokingColorRGB.b       //
    );
}

void renderer::endText() {
    // make sure text object parameters are set before unsetting them
    ASSERT(stateStack.back().textState.textObjectParams.has_value());
    stateStack.back().textState.textObjectParams = {};
}

void renderer::beginText() {
    // only one BT ... ET can be open at a time
    ASSERT(!stateStack.back().textState.textObjectParams.has_value());
    stateStack.back().textState.textObjectParams = std::optional(TextObjectState());
}

void renderer::pushGraphicsState() { stateStack.emplace_back(); }

void renderer::popGraphicsState() { stateStack.pop_back(); }

void renderer::moveStartOfNextLine(Operator *op) { TODO("implement move start of next line"); }

void renderer::setTextFont(Operator *op) {
    stateStack.back().textState.textFontSize = op->data.Tf_SetTextFont.fontSize;

    auto fontName      = std::string_view(op->data.Tf_SetTextFont.fontNameData, op->data.Tf_SetTextFont.fontNameLength);
    fontName           = fontName.substr(1); // remove leading "/"
    auto resourcesDict = page->resources();
    auto &values       = resourcesDict->values;
    auto itr           = values.find("Font");
    if (itr == values.end()) {}

    auto fontsDict = page->document.get<Dictionary>(itr->second);
    for (auto &entry : fontsDict->values) {
        if (entry.first != fontName) {
            continue;
        }

        auto fontDict = page->document.get<Dictionary>(entry.second);
        auto type     = fontDict->values["Subtype"]->as<Name>()->value;
        if (type == "TrueType") {
            loadTrueTypeFont(fontDict);
        } else {
            TODO("implement loading of other fonts");
            ASSERT(false);
        }
    }
}

void renderer::showText(Operator *op) {
    auto values = op->data.TJ_ShowOneOrMoreTextStrings.objects->values;
    for (auto value : values) {
        if (value->is<Integer>()) {
            std::cout << "Integer" << std::endl;
        } else if (value->is<HexadecimalString>()) {
            std::cout << "HexadecimalString: ";
            auto str = value->as<HexadecimalString>()->to_string();
            std::cout << str << std::endl;
        }
    }
}

void renderer::loadTrueTypeFont(Dictionary *fontDict) {
    stateStack.back().textState.textFont.type          = FontType::TRUE_TYPE;
    auto font                                          = fontDict->as<TrueTypeFont>();
    stateStack.back().textState.textFont.font.trueType = font;

    // TODO inspect font correctly
    std::cout << font->name().has_value() << std::endl;
    std::cout << font->baseFont() << std::endl;
    std::cout << font->firstChar() << std::endl;
    std::cout << font->lastChar() << std::endl;
    std::cout << font->widths(page->document) << std::endl;
    std::cout << font->fontDescriptor(page->document) << std::endl;
    std::cout << font->encoding(page->document).has_value() << std::endl;
    std::cout << font->toUnicode(page->document).has_value() << std::endl;
}

} // namespace pdf
