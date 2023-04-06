#include "operator_traverser.h"

namespace pdf {

void OperatorTraverser::traverse() {
    // TODO set graphics state to default values

    auto streams = page.content_streams();
    ASSERT(!streams.empty());
    for (auto stream : streams) {
        currentContentStream = stream;
        stream->for_each_operator(page.document.allocator, [this](Operator *op) {
            apply_operator(op);
            return ForEachResult::CONTINUE;
        });
    }
}

void OperatorTraverser::apply_operator(Operator *op) {
//    spdlog::info("{}", operatorTypeToString(op->type));
    switch (op->type) {
    case Operator::Type::w_SetLineWidth:
        state().lineWidth = op->data.w_SetLineWidth.lineWidth;
        break;
    case Operator::Type::q_PushGraphicsState:
        pushGraphicsState();
        break;
    case Operator::Type::Q_PopGraphicsState:
        popGraphicsState();
        break;
    case Operator::Type::re_AppendRectangle:
        appendRectangle();
        break;
    case Operator::Type::Wx_ModifyClippingPathUsingEvenOddRule:
        modifyClippingPathUsingEvenOddRule();
        break;
    case Operator::Type::n_EndPathWithoutFillingOrStroking:
        endPathWithoutFillingOrStroking();
        break;
    case Operator::Type::rg_SetNonStrokingColorRGB:
        setNonStrokingColor(op);
        break;
    case Operator::Type::cm_ModifyCurrentTransformationMatrix:
        modifyCurrentTransformationMatrix(op);
        break;
    case Operator::Type::BT_BeginText:
        beginText();
        break;
    case Operator::Type::ET_EndText:
        endText();
        break;
    case Operator::Type::Td_MoveStartOfNextLine:
        moveStartOfNextLine(op);
        break;
    case Operator::Type::Tf_SetTextFontAndSize:
        setTextFontAndSize(op);
        break;
    case Operator::Type::TJ_ShowOneOrMoreTextStrings:
        on_show_text(op);
        break;
    case Operator::Type::Do_PaintXObject:
        on_do(op);
        break;
    default:
        // TODO unknown operator
        spdlog::warn("OperatorTraverser::apply_operator() - unknown operator {}", operatorTypeToString(op->type));
    }
}

void OperatorTraverser::modifyCurrentTransformationMatrix(Operator *op) {
    const auto &m1 = op->data.cm_ModifyCurrentTransformationMatrix.matrix;
    auto matrix    = Cairo::Matrix(m1[0], m1[1], m1[2], m1[3], m1[4], m1[5]);
    state().currentTransformationMatrix.multiply(state().currentTransformationMatrix, matrix);
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
}

void OperatorTraverser::pushGraphicsState() { stateStack.push_back(state()); }

void OperatorTraverser::popGraphicsState() { stateStack.pop_back(); }

void OperatorTraverser::moveStartOfNextLine(Operator *op) {
    auto tmp             = Cairo::identity_matrix();
    auto startOfNextLine = op->data.Td_MoveStartOfNextLine;
    tmp.translate(startOfNextLine.x, startOfNextLine.y);

    ASSERT(state().textState.textObjectParams.has_value());

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
    textRenderMatrix.scale(textState.textFontSize, -textState.textFontSize);
    textRenderMatrix.translate(0, textState.textRiseUnscaled);
    return textRenderMatrix;
}

} // namespace pdf
