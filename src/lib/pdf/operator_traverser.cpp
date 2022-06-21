#include "operator_traverser.h"

namespace pdf {

void OperatorTraverser::traverse() {
    // TODO set graphics state to default values

    auto streams = page.content_streams();
    ASSERT(!streams.empty());
    for (auto s : streams) {
        currentContentStream = s;
        s->for_each_operator(page.document.allocator, [this](Operator *op) {
            apply_operator(op);
            return ForEachResult::CONTINUE;
        });
    }
}

void OperatorTraverser::apply_operator(Operator *op) {
    if (op->type == Operator::Type::w_SetLineWidth) {
        state().lineWidth = op->data.w_SetLineWidth.lineWidth;
    } else if (op->type == Operator::Type::q_PushGraphicsState) {
        pushGraphicsState();
    } else if (op->type == Operator::Type::Q_PopGraphicsState) {
        popGraphicsState();
    } else if (op->type == Operator::Type::re_AppendRectangle) {
        appendRectangle();
    } else if (op->type == Operator::Type::Wx_ModifyClippingPathUsingEvenOddRule) {
        modifyClippingPathUsingEvenOddRule();
    } else if (op->type == Operator::Type::n_EndPathWithoutFillingOrStroking) {
        endPathWithoutFillingOrStroking();
    } else if (op->type == Operator::Type::rg_SetNonStrokingColorRGB) {
        setNonStrokingColor(op);
    } else if (op->type == Operator::Type::BT_BeginText) {
        beginText();
    } else if (op->type == Operator::Type::ET_EndText) {
        endText();
    } else if (op->type == Operator::Type::Td_MoveStartOfNextLine) {
        moveStartOfNextLine(op);
    } else if (op->type == Operator::Type::Tf_SetTextFontAndSize) {
        setTextFontAndSize(op);
    } else if (op->type == Operator::Type::TJ_ShowOneOrMoreTextStrings) {
        on_show_text(op);
    } else if (op->type == Operator::Type::Do_PaintXObject) {
        on_do(op);
    } else {
        // TODO unknown operator
        spdlog::trace("OperatorTraverser::apply_operator() - unknown operator {}", op->type);
    }
}

void OperatorTraverser::appendRectangle() const { // TODO append rectangle to the current path
}

void OperatorTraverser::modifyClippingPathUsingEvenOddRule() const { // TODO clipping path modification
}

void OperatorTraverser::endPathWithoutFillingOrStroking() const {
    // TODO this is a path painting no-op
    //  it does however set the clipping path, if a clipping path operator was used before it
}

void OperatorTraverser::setNonStrokingColor(Operator *op) {
    state().nonStrokingColor = Color::rgb(      //
          op->data.rg_SetNonStrokingColorRGB.r, //
          op->data.rg_SetNonStrokingColorRGB.g, //
          op->data.rg_SetNonStrokingColorRGB.b  //
    );
}

void OperatorTraverser::endText() {
    // make sure text object parameters are set before unsetting them
    ASSERT(state().textState.textObjectParams.has_value());
    state().textState.textObjectParams = {};
}

void OperatorTraverser::beginText() {
    // only one BT ... ET can be open at a time
    ASSERT(!state().textState.textObjectParams.has_value());
    state().textState.textObjectParams = std::optional(TextObjectState());

    // TODO this seems wrong (how can we flip the coordinate space on the y axis?)
    state().textState.textObjectParams.value().textLineMatrix.translate(0, page.height());
}

void OperatorTraverser::pushGraphicsState() { stateStack.emplace_back(); }

void OperatorTraverser::popGraphicsState() { stateStack.pop_back(); }

void OperatorTraverser::moveStartOfNextLine(Operator *op) {
    auto tmp = Cairo::identity_matrix();
    // TODO this '-' seems wrong (how can we flip the coordinate space on the y axis?)
    tmp.translate(op->data.Td_MoveStartOfNextLine.x, -op->data.Td_MoveStartOfNextLine.y);

    auto currentLineMatrix = state().textState.textObjectParams.value().textLineMatrix;
    auto newLineMatrix     = tmp * currentLineMatrix;

    state().textState.textObjectParams.value().textLineMatrix = newLineMatrix;
    state().textState.textObjectParams.value().textMatrix     = newLineMatrix;
}

void OperatorTraverser::setTextFontAndSize(Operator *op) {
    state().textState.textFontSize = op->data.Tf_SetTextFontAndSize.fontSize;

    auto fontMapOpt = page.resources()->fonts(page.document);
    if (!fontMapOpt.has_value()) {
        // TODO add logging
        return;
    }

    auto fontName = std::string(op->data.Tf_SetTextFontAndSize.font_name());
    auto fontOpt  = fontMapOpt.value()->get(page.document, fontName);
    if (!fontOpt.has_value()) {
        // TODO add logging
        return;
    }

    auto font                       = fontOpt.value();
    auto fontFace                   = font->load_font_face(page.document);
    state().textState.textFont.font = font;
    if (fontFace != nullptr) {
        state().textState.textFont.ftFace    = fontFace;
        state().textState.textFont.cairoFace = Cairo::FtFontFace::create(fontFace, 0);
    }
}

Cairo::Matrix OperatorTraverser::font_matrix() const {
    const auto &textState = state().textState;
    auto textRenderMatrix = Cairo::identity_matrix();
    textRenderMatrix.scale(textState.textFontSize, textState.textFontSize);
    textRenderMatrix.translate(0, textState.textRiseUnscaled);

    return textRenderMatrix * textState.textObjectParams.value().textMatrix;
}

} // namespace pdf
