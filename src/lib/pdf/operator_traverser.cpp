#include "operator_traverser.h"

#include <cairo/cairo-ft.h>

#include "pdf/page.h"

namespace pdf {

OperatorTraverser::OperatorTraverser(Page &_page)
    : page(_page), stateStack(_page.document.allocator), textBlocks(_page.document.allocator),
      images(_page.document.allocator) {
    stateStack.emplace_back();
}

void OperatorTraverser::traverse(cairo_t *crIn) {
    if (!dirty) {
        // simply draw the recorded sequence again
        cairo_set_source_surface(crIn, recordingSurface, 0.0, 0.0);
        cairo_paint(crIn);
        return;
    }

    recordingSurface = cairo_recording_surface_create(CAIRO_CONTENT_COLOR, nullptr);
    auto cr          = cairo_create(recordingSurface);

    images.clear();
    textBlocks.clear();

    cairo_save(cr);

    cairo_set_miter_limit(cr, 10.0);

    // move (0,0) from the top-left to the bottom-left corner of the page
    // and make positive y-axis extend vertically upward
    auto pageHeight       = page.attr_height();
    cairo_matrix_t matrix = {};
    cairo_matrix_init(&matrix, 1.0, 0.0, 0.0, -1.0, 0.0, pageHeight);
    cairo_transform(cr, &matrix);

    auto cropBox = page.attr_crop_box();
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_rectangle(cr, 0, 0, cropBox->width(), cropBox->height());
    cairo_fill(cr);

    auto streams = page.content_streams();
    ASSERT(!streams.empty());

    for (auto stream : streams) {
        currentContentStream = stream;
        stream->for_each_operator(page.document.allocator, [this, &cr](Operator *op) {
            apply_operator(cr, op);
            return ForEachResult::CONTINUE;
        });
    }

    cairo_restore(cr);

    cairo_set_source_surface(crIn, recordingSurface, 0.0, 0.0);
    cairo_paint(crIn);

    dirty = false;
}

void OperatorTraverser::apply_operator(cairo_t *cr, Operator *op) {
    //    spdlog::info("{}", operatorTypeToString(op->type));
    switch (op->type) {
    case Operator::Type::w_SetLineWidth:
        cairo_set_line_width(cr, op->data.w_SetLineWidth.lineWidth);
        break;
    case Operator::Type::q_PushGraphicsState:
        pushGraphicsState(cr);
        break;
    case Operator::Type::Q_PopGraphicsState:
        popGraphicsState(cr);
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
        setNonStrokingColor(cr, op);
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
        setTextFontAndSize(cr, op);
        break;
    case Operator::Type::TJ_ShowOneOrMoreTextStrings:
        showText(cr, op);
        break;
    case Operator::Type::Do_PaintXObject:
        onDo(cr, op);
        break;
    default:
        // TODO unknown operator
        // spdlog::warn("OperatorTraverser::apply_operator() - unknown operator {}", operatorTypeToString(op->type));
        break;
    }
}

void OperatorTraverser::modifyCurrentTransformationMatrix(Operator *op) {
    const auto &m         = op->data.cm_ModifyCurrentTransformationMatrix.matrix;
    cairo_matrix_t matrix = {};
    cairo_matrix_init(&matrix, 1.0, m[1], m[2], 1, m[4], m[5]);
    cairo_matrix_multiply(&state().ctm, &state().ctm, &matrix);
}

void OperatorTraverser::appendRectangle() const {
    // TODO append rectangle to the current path
}

void OperatorTraverser::modifyClippingPathUsingEvenOddRule() const {
    // TODO clipping path modification
}

void OperatorTraverser::endPathWithoutFillingOrStroking() const {
    // TODO this is a path painting no-op
    //  it does however set the clipping path, if a clipping path operator was used before it
}

void OperatorTraverser::setNonStrokingColor(cairo_t *cr, Operator *op) {
    cairo_set_source_rgb(cr,                                   //
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

void OperatorTraverser::pushGraphicsState(cairo_t *cr) {
    cairo_save(cr);
    stateStack.push_back(state());
}

void OperatorTraverser::popGraphicsState(cairo_t *cr) {
    cairo_restore(cr);
    stateStack.pop_back();
}

void OperatorTraverser::moveStartOfNextLine(Operator *op) {
    cairo_matrix_t tmp = {};
    cairo_matrix_init_identity(&tmp);
    auto startOfNextLine = op->data.Td_MoveStartOfNextLine;
    cairo_matrix_translate(&tmp, startOfNextLine.x, startOfNextLine.y);

    ASSERT(state().textState.textObjectParams.has_value());

    auto currentLineMatrix       = state().textState.textObjectParams.value().textLineMatrix;
    cairo_matrix_t newLineMatrix = {};
    cairo_matrix_multiply(&newLineMatrix, &tmp, &currentLineMatrix);

    state().textState.textObjectParams.value().textLineMatrix = newLineMatrix;
    state().textState.textObjectParams.value().textMatrix     = newLineMatrix;
}

void OperatorTraverser::setTextFontAndSize(cairo_t *cr, Operator *op) {
    auto &textState        = state().textState;
    textState.textFontSize = op->data.Tf_SetTextFontAndSize.fontSize;

    cairo_matrix_t textRenderMatrix = {};
    cairo_matrix_init_identity(&textRenderMatrix);
    cairo_matrix_scale(&textRenderMatrix, textState.textFontSize, -textState.textFontSize);
    cairo_matrix_translate(&textRenderMatrix, 0, textState.textRiseUnscaled);
    cairo_set_font_matrix(cr, &textRenderMatrix);

    auto fontMapOpt = page.attr_resources()->fonts(page.document);
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
        state().textState.textFont.ftFace = fontFace;

        auto cairo_ff = cairo_ft_font_face_create_for_ft_face(fontFace, 0);
        cairo_set_font_face(cr, cairo_ff);
    }
}

void OperatorTraverser::showText(cairo_t *cr, Operator *op) {
    const auto &textState = state().textState;
    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);

    auto scaledFont = cairo_get_scaled_font(cr);
    auto values     = op->data.TJ_ShowOneOrMoreTextStrings.objects->values;
    auto glyphs     = std::vector<cairo_glyph_t>();
    double xOffset  = 0;
    for (auto value : values) {
        if (value->is<Integer>()) {
            auto i = value->as<Integer>();
            xOffset -= static_cast<double>(i->value) / 1000.0;
        } else if (value->is<HexadecimalString>()) {
            auto str = value->as<HexadecimalString>()->to_string();
            for (char c : str) {
                auto i              = static_cast<uint8_t>(c);
                cairo_glyph_t glyph = {.index = i, .x = xOffset, .y = 0.0};
                glyphs.push_back(glyph);

                cairo_text_extents_t extents = {};
                cairo_scaled_font_glyph_extents(scaledFont, &glyph, 1, &extents);
                xOffset += static_cast<double>(extents.x_advance);
            }
        } else if (value->is<LiteralString>()) {
            const auto &utf8                 = value->as<LiteralString>()->value;
            cairo_glyph_t *newGlyphs         = nullptr;
            int numNewGlyphs                 = 0;
            cairo_text_cluster_t *clusters   = nullptr;
            int numClusters                  = 0;
            cairo_text_cluster_flags_t flags = {};
            const auto status =
                  cairo_scaled_font_text_to_glyphs(scaledFont, xOffset, 0.0, utf8.data(), utf8.size(), &newGlyphs,
                                                   &numNewGlyphs, &clusters, &numClusters, &flags);
            if (status != CAIRO_STATUS_SUCCESS) {
                continue;
            }

            for (int i = 0; i < numNewGlyphs; i++) {
                const auto &glyph = newGlyphs[i];
                glyphs.push_back(glyph);

                cairo_text_extents_t extents = {};
                cairo_scaled_font_glyph_extents(scaledFont, &glyph, 1, &extents);
                xOffset += static_cast<double>(extents.x_advance);
            }
        }
    }

    cairo_save(cr);
    cairo_transform(cr, &textState.textObjectParams.value().textMatrix);
    cairo_show_glyphs(cr, glyphs.data(), glyphs.size());
    cairo_restore(cr);

    // TODO integrate the code below into the loop above

    glyphs.clear();

    const TextFont &textFont = state().textState.textFont;
    auto cmapOpt             = textFont.font->cmap(page.document);

    double offsetX = 0.0;
    std::string text;
    for (auto value : op->data.TJ_ShowOneOrMoreTextStrings.objects->values) {
        if (value->is<HexadecimalString>()) {
            if (cmapOpt.has_value()) {
                text += cmapOpt.value()->map_char_codes(value->as<HexadecimalString>());
            }
            auto str = value->as<HexadecimalString>()->to_string();
            for (char c : str) {
                auto i              = static_cast<uint8_t>(c);
                cairo_glyph_t glyph = {.index = i, .x = offsetX, .y = 0.0};
                glyphs.push_back(glyph);

                cairo_text_extents_t extents = {};
                cairo_scaled_font_glyph_extents(scaledFont, &glyph, 1, &extents);
                offsetX += static_cast<double>(extents.x_advance);
            }
        } else if (value->is<LiteralString>()) {
            text += value->as<LiteralString>()->value;
        } else if (value->is<Integer>()) {
            offsetX -= static_cast<double>(value->as<Integer>()->value) / 1000.0;
        }
    }

    double x = 0.0;
    double y = 0.0;
    cairo_matrix_transform_point(&textState.textObjectParams->textLineMatrix, &x, &y);

    cairo_text_extents_t extents = {};
    cairo_scaled_font_glyph_extents(scaledFont, glyphs.data(), static_cast<int>(glyphs.size()), &extents);
    textBlocks.push_back({
          .page   = &page,
          .text   = text,
          .x      = x,
          .y      = y,
          .width  = extents.width,
          .height = extents.height,
          .op     = op,
          .cs     = currentContentStream,
    });
}

void OperatorTraverser::onDo(cairo_t *cr, Operator *op) {
    auto pageImageResult = pdf::PageImage::create(page, state().ctm, op, currentContentStream);
    if (pageImageResult.has_error()) {
        return;
    }

    auto pageImage = pageImageResult.value();
    images.push_back(pageImage);

    auto image               = pageImage.image;
    auto pixels              = image->decode(page.document.allocator);
    auto width               = image->width();
    auto height              = image->height();
    auto bitsPerComponentOpt = image->bits_per_component();
    if (!bitsPerComponentOpt.has_value()) {
        return;
    }

    const auto stride   = cairo_format_stride_for_width(CAIRO_FORMAT_RGB24, width);
    auto currentRowSize = static_cast<int32_t>((bitsPerComponentOpt.value()->value * 3 * width) / 32.0 * 4.0);

    auto tempArena = page.document.allocator.temporary();
    auto pBuf      = tempArena.arena().push(stride * height);
    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            auto pBufIndex      = (height - row - 1) * stride + col * 4;
            auto pixelsIndex    = static_cast<int32_t>(row * currentRowSize + col * 3);
            pBuf[pBufIndex + 0] = pixels[pixelsIndex + 2];
            pBuf[pBufIndex + 1] = pixels[pixelsIndex + 1];
            pBuf[pBufIndex + 2] = pixels[pixelsIndex + 0];
            pBuf[pBufIndex + 3] = 0;
        }
    }

    auto surface = cairo_image_surface_create_for_data(pBuf, CAIRO_FORMAT_RGB24, width, height, stride);
    cairo_set_source_surface(cr, surface, pageImage.xOffset, pageImage.yOffset);
    cairo_paint(cr);
    cairo_surface_finish(surface);
}

ValueResult<PageImage> PageImage::create(Page &page, const cairo_matrix_t &ctm, Operator *op, ContentStream *cs) {
    const auto &xObjectName = op->data.Do_PaintXObject.name->value;

    const auto xObjectMapOpt = page.attr_resources()->x_objects(page.document);
    if (!xObjectMapOpt.has_value()) {
        return ValueResult<PageImage>::error("failed to get XObject map");
    }

    const auto &xObjectMap   = xObjectMapOpt.value();
    const auto &xObjectKey   = xObjectName.substr(1, xObjectName.size() - 1);
    const auto xObjectRefOpt = xObjectMap->find<IndirectReference>(xObjectKey);
    const auto xObjectOpt    = page.document.get<Stream>(xObjectRefOpt);
    if (!xObjectOpt.has_value()) {
        return ValueResult<PageImage>::error("failed to find XObject with XObjectKey={}", xObjectKey);
    }

    const auto xObject = xObjectOpt.value();
    const auto subtype = xObject->dictionary->must_find<Name>("Subtype");
    if (subtype->value != "Image") {
        return ValueResult<PageImage>::error("XObject is not an image");
    }

    double xOffset = 0.0;
    double yOffset = 0.0;
    cairo_matrix_transform_point(&ctm, &xOffset, &yOffset);
    auto pageImage = PageImage(&page, xObjectKey, xOffset, yOffset, xObject->as<XObjectImage>(), op, cs);
    return ValueResult<PageImage>::ok(pageImage);
}

void PageImage::move(Document &document, double offsetX, double offsetY) const {
    std::stringstream ss;

    auto decoded            = cs->decode(document.allocator);
    auto bytesUntilOperator = op->content.data() - decoded.data();

    // write everything up to the operator we want to wrap
    ss << decoded.substr(0, bytesUntilOperator);

    if (op->content[0] != ' ') {
        ss << " ";
    }

    // TODO operator stream, if we applied such a move operation previously and just edit that one instead

    // save state
    ss << "q ";

    // apply offset
    ss << "1 0 0 1 ";
    ss << offsetX;
    ss << " ";
    ss << offsetY;
    ss << " cm ";

    // write operator
    ss << op->content;

    // restore state
    ss << " Q";

    ss << decoded.substr(bytesUntilOperator + op->content.size());

    cs->encode(document.allocator, ss.str());

    page->traverser.dirty = true;

    spdlog::info("Moved image '{}' by x={} and y={}", name, offsetX, offsetY);
}

void TextBlock::move(Document &document, double offsetX, double offsetY) const {
    // BT 56.8 724.1 Td /F1 12 Tf            [<01>-2<02>1<03>2<03>2<0405>17<06>76<040708>]TJ              ET Q Q
    // BT 56.8 724.1 Td /F1 12 Tf _x_ _y_ Td [<01>-2<02>1<03>2<03>2<0405>17<06>76<040708>]TJ -_x_ -_y_ Td ET Q Q
    std::stringstream ss;

    // write everything up to the operator we want to wrap
    auto decoded = cs->decode(document.allocator);
    ss << decoded.substr(0, op->content.data() - decoded.data());

    // wrap operator with offset
    if (op->content[0] != ' ') {
        ss << " ";
    }
    ss << offsetX;
    ss << " ";
    ss << -offsetY;
    ss << " Td ";

    // write operator
    ss << op->content;

    // wrap operator with negative offset
    ss << " ";
    ss << -offsetX;
    ss << " ";
    ss << offsetY;
    ss << " Td ";

    ss << decoded.substr(op->content.data() - decoded.data() + op->content.size());

    cs->encode(document.allocator, ss.str());

    page->traverser.dirty = true;

    spdlog::info("Moved text block '{}' by x={} and y={}", text, offsetX, offsetY);
}

} // namespace pdf
