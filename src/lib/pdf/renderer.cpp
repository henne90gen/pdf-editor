#include "renderer.h"

#include <spdlog/spdlog.h>

#include "helper/util.h"
#include "operator_parser.h"

namespace pdf {

void Renderer::render(const Cairo::RefPtr<Cairo::Context> &cr) {
    // TODO set graphics state to default values
    // NOTE the ctm of cairo already translates into the correct coordinate system, this has to be preserved

    auto cropBox = page.crop_box();
    cr->set_source_rgb(1, 1, 1);
    cr->rectangle(0, 0, cropBox->width(), cropBox->height());
    cr->fill();

    auto contentStreams = page.content_streams();
    ASSERT(!contentStreams.empty());
    render(cr, contentStreams);
}

void Renderer::render(const Cairo::RefPtr<Cairo::Context> &cr, const std::vector<ContentStream *> &streams) {
    for (auto s : streams) {
        s->for_each_operator(page.document.allocator, [this, &cr](Operator *op) {
            apply_operator(cr, op);

            return ForEachResult::CONTINUE;
        });
    }
}

void Renderer::apply_operator(const Cairo::RefPtr<Cairo::Context> &cr, Operator *op) {
    if (op->type == Operator::Type::w_SetLineWidth) {
        stateStack.back().lineWidth = op->data.w_SetLineWidth.lineWidth;
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
        showText(cr, op);
    } else {
        // TODO unknown operator
    }
}

void Renderer::appendRectangle() const { // TODO append rectangle to the current path
}

void Renderer::modifyClippingPathUsingEvenOddRule() const { // TODO clipping path modification
}

void Renderer::endPathWithoutFillingOrStroking() const {
    // TODO this is a path painting no-op
    //  it does however set the clipping path, if a clipping path operator was used before it
}

void Renderer::setNonStrokingColor(Operator *op) {
    stateStack.back().nonStrokingColor = Color::rgb( //
          op->data.rg_SetNonStrokingColorRGB.r,      //
          op->data.rg_SetNonStrokingColorRGB.g,      //
          op->data.rg_SetNonStrokingColorRGB.b       //
    );
}

void Renderer::endText() {
    // make sure text object parameters are set before unsetting them
    ASSERT(stateStack.back().textState.textObjectParams.has_value());
    stateStack.back().textState.textObjectParams = {};
}

void Renderer::beginText() {
    // only one BT ... ET can be open at a time
    ASSERT(!stateStack.back().textState.textObjectParams.has_value());
    stateStack.back().textState.textObjectParams = std::optional(TextObjectState());
}

void Renderer::pushGraphicsState() { stateStack.emplace_back(); }

void Renderer::popGraphicsState() { stateStack.pop_back(); }

void Renderer::moveStartOfNextLine(Operator *op) {
    auto tmp = Cairo::identity_matrix();
    tmp.translate(op->data.Td_MoveStartOfNextLine.x, page.height() - op->data.Td_MoveStartOfNextLine.y);

    auto currentLineMatrix = stateStack.back().textState.textObjectParams.value().textLineMatrix;
    auto newLineMatrix     = tmp * currentLineMatrix;

    stateStack.back().textState.textObjectParams.value().textLineMatrix = newLineMatrix;
    stateStack.back().textState.textObjectParams.value().textMatrix     = newLineMatrix;
}

void Renderer::setTextFontAndSize(Operator *op) {
    stateStack.back().textState.textFontSize = op->data.Tf_SetTextFontAndSize.fontSize;

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

    auto font                                 = fontOpt.value();
    auto fontFace                             = font->load_font_face(page.document);
    stateStack.back().textState.textFont.font = font;
    if (fontFace != nullptr) {
        stateStack.back().textState.textFont.ftFace    = fontFace;
        stateStack.back().textState.textFont.cairoFace = Cairo::FtFontFace::create(fontFace, 0);
    }
}

void Renderer::showText(const Cairo::RefPtr<Cairo::Context> &cr, Operator *op) {
    auto &textState       = stateStack.back().textState;
    auto textRenderMatrix = Cairo::identity_matrix();
    textRenderMatrix.scale(textState.textFontSize, textState.textFontSize);
    textRenderMatrix.translate(0, textState.textRiseUnscaled);

    auto fontMatrix = textRenderMatrix * textState.textObjectParams.value().textMatrix;
    cr->set_font_matrix(fontMatrix);
    cr->set_font_face(textState.textFont.cairoFace);
    cr->set_source_rgb(0.0, 0.0, 0.0);

    auto scaledFont = cr->get_scaled_font();
    auto values     = op->data.TJ_ShowOneOrMoreTextStrings.objects->values;
    auto glyphs     = std::vector<Cairo::Glyph>();
    double xOffset  = 0;
    for (auto value : values) {
        if (value->is<Integer>()) {
            auto i = value->as<Integer>();
            xOffset -= static_cast<double>(i->value) / 1000.0;
        } else if (value->is<HexadecimalString>()) {
            auto str = value->as<HexadecimalString>()->to_string();
            for (char c : str) {
                auto i             = static_cast<uint8_t>(c);
                Cairo::Glyph glyph = {.index = i, .x = xOffset, .y = 0.0};
                glyphs.push_back(glyph);

                Cairo::TextExtents extents;
                cairo_scaled_font_glyph_extents(scaledFont->cobj(), &glyph, 1, &extents);
                xOffset += static_cast<double>(extents.x_advance);
            }
        } else if (value->is<LiteralString>()) {
            auto utf8     = std::string(value->as<LiteralString>()->value());
            auto clusters = std::vector<Cairo::TextCluster>();
            Cairo::TextClusterFlags flags;
            std::vector<Cairo::Glyph> newGlyphs = {};
            scaledFont->text_to_glyphs(xOffset, 0.0, utf8, newGlyphs, clusters, flags);
            for (auto &g : newGlyphs) {
                glyphs.push_back(g);
                Cairo::TextExtents extents;
                cairo_scaled_font_glyph_extents(scaledFont->cobj(), &g, 1, &extents);
                xOffset += static_cast<double>(extents.x_advance);
            }
        }
    }

    cr->show_glyphs(glyphs);
}

} // namespace pdf
