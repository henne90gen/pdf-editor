#include "renderer.h"

#include "operator_parser.h"

void pdf::renderer::render(pdf::Page *page) {
    auto contentsOpt = page->contents();
    if (!contentsOpt.has_value()) {
        return;
    }

    // TODO set graphics state to default values

    auto content = contentsOpt.value();
    if (content->is<IndirectReference>()) {
        content = page->document.resolve(content->as<IndirectReference>())->object;
    }

    if (content->is<Stream>()) {
        render({content->as<Stream>()});
    } else if (content->is<Array>()) {
        auto arr     = content->as<Array>();
        auto streams = std::vector<Stream *>(arr->values.size());
        for (auto val : arr->values) {
            streams.push_back(val->as<Stream>());
        }
        render(streams);
    } else {
        // NOTE this should never be reached
        ASSERT(false);
    }
}

void pdf::renderer::render(const std::vector<Stream *> &streams) {
    for (auto s : streams) {
        auto textProvider   = StringTextProvider(s->to_string());
        auto lexer          = TextLexer(textProvider);
        auto operatorParser = OperatorParser(lexer);
        Operator *op        = operatorParser.getOperator();
        while (op != nullptr) {
            if (op->type == Operator::Type::w_SetLineWidth) {
                stateStack.back().lineWidth = op->data.w_SetLineWidth.lineWidth;
            } else if (op->type == Operator::Type::q_PushGraphicsState) {
                stateStack.emplace_back();
            } else if (op->type == Operator::Type::Q_PopGraphicsState) {
                stateStack.pop_back();
            } else if (op->type == Operator::Type::re_AppendRectangle) {
                // TODO append rectangle to the current path
            } else if (op->type == Operator::Type::Wx_ModifyClippingPathUsingEvenOddRule) {
                // TODO implement this
            } else if (op->type == Operator::Type::n_EndPathWithoutFillingOrStroking) {
                // TODO this is a path painting no-op
                //  it does however set the clipping path, if a clipping path operator was used before it
            } else if (op->type == Operator::Type::rg_SetNonStrokingColorRGB) {
                stateStack.back().nonStrokingColor = Color::rgb( //
                      op->data.rg_SetNonStrokingColorRGB.r,      //
                      op->data.rg_SetNonStrokingColorRGB.g,      //
                      op->data.rg_SetNonStrokingColorRGB.b       //
                );
            } else if (op->type == Operator::Type::BT_BeginText) {
                // only one BT ... ET can be open at a time
                ASSERT(!stateStack.back().textState.textObjectParams.has_value());
                stateStack.back().textState.textObjectParams = std::optional(TextObjectState());
            } else if (op->type == Operator::Type::ET_EndText) {
                // make sure text object parameters are set before unsetting them
                ASSERT(stateStack.back().textState.textObjectParams.has_value());
                stateStack.back().textState.textObjectParams = {};
            } else if (op->type == Operator::Type::Td_MoveStartOfNextLine) {
                // TODO implement this
            } else if (op->type == Operator::Type::Tf_SetTextFont) {
                // TODO implement this
            } else if (op->type == Operator::Type::TJ_ShowOneOrMoreTextStrings) {
                // TODO implement this
            } else {
                std::cout << "Found unknown operator: " << op->type << std::endl;
            }
            op = operatorParser.getOperator();
        }
    }
}
