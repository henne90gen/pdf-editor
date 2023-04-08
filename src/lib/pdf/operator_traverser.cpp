#include "operator_traverser.h"

#include "pdf/page.h"

namespace pdf {

void OperatorTraverser::traverse() {
    if (!dirty) {
        return;
    }

    images.clear();
    textBlocks.clear();

    cr->save();

    cr->set_miter_limit(10.0);

    // move (0,0) from the top-left to the bottom-left corner of the page
    // and make positive y-axis extend vertically upward
    auto pageHeight = page.attr_height();
    auto matrix     = Cairo::Matrix(1.0, 0.0, 0.0, -1.0, 0.0, pageHeight);
    cr->transform(matrix);

    auto cropBox = page.attr_crop_box();
    cr->set_source_rgb(1, 1, 1);
    cr->rectangle(0, 0, cropBox->width(), cropBox->height());
    cr->fill();

    auto streams = page.content_streams();
    ASSERT(!streams.empty());
    for (auto stream : streams) {
        currentContentStream = stream;
        stream->for_each_operator(page.document.allocator, [this](Operator *op) {
            apply_operator(op);
            return ForEachResult::CONTINUE;
        });
    }

    cr->restore();

    dirty = false;
}

void OperatorTraverser::apply_operator(Operator *op) {
    //    spdlog::info("{}", operatorTypeToString(op->type));
    switch (op->type) {
    case Operator::Type::w_SetLineWidth:
        cr->set_line_width(op->data.w_SetLineWidth.lineWidth);
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
        showText(op);
        break;
    case Operator::Type::Do_PaintXObject:
        onDo(op);
        break;
    default:
        // TODO unknown operator
        spdlog::warn("OperatorTraverser::apply_operator() - unknown operator {}", operatorTypeToString(op->type));
    }
}

void OperatorTraverser::modifyCurrentTransformationMatrix(Operator *op) {
    const auto &m = op->data.cm_ModifyCurrentTransformationMatrix.matrix;
    auto matrix   = Cairo::Matrix(1, m[1], m[2], 1, m[4], m[5]);
    state().ctm.multiply(state().ctm, matrix);
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

void OperatorTraverser::setNonStrokingColor(Operator *op) {
    cr->set_source_rgb(                         //
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

void OperatorTraverser::pushGraphicsState() {
    cr->save();
    stateStack.push_back(state());
}

void OperatorTraverser::popGraphicsState() {
    cr->restore();
    stateStack.pop_back();
}

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
    auto &textState        = state().textState;
    textState.textFontSize = op->data.Tf_SetTextFontAndSize.fontSize;

    auto textRenderMatrix = Cairo::identity_matrix();
    textRenderMatrix.scale(textState.textFontSize, -textState.textFontSize);
    textRenderMatrix.translate(0, textState.textRiseUnscaled);
    cr->set_font_matrix(textRenderMatrix);

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
        cr->set_font_face(Cairo::FtFontFace::create(fontFace, 0));
    }
}

void OperatorTraverser::showText(Operator *op) {
    const auto &textState = state().textState;
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
            const auto &utf8 = value->as<LiteralString>()->value;
            auto clusters    = std::vector<Cairo::TextCluster>();
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

    cr->save();
    cr->transform(textState.textObjectParams.value().textMatrix);
    cr->show_glyphs(glyphs);
    cr->restore();

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
                auto i             = static_cast<uint8_t>(c);
                Cairo::Glyph glyph = {.index = i, .x = offsetX, .y = 0.0};
                glyphs.push_back(glyph);

                Cairo::TextExtents extents;
                cairo_scaled_font_glyph_extents(scaledFont->cobj(), &glyph, 1, &extents);
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
    textState.textObjectParams->textLineMatrix.transform_point(x, y);

    Cairo::TextExtents extents = {};
    cairo_scaled_font_glyph_extents(scaledFont->cobj(), glyphs.data(), static_cast<int>(glyphs.size()), &extents);
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

void OperatorTraverser::onDo(Operator *op) {
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

    auto stride         = Cairo::ImageSurface::format_stride_for_width(Cairo::ImageSurface::Format::RGB24, width);
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

    auto surface = Cairo::ImageSurface::create(pBuf, Cairo::ImageSurface::Format::RGB24, width, height, stride);
    cr->set_source(surface, pageImage.xOffset, pageImage.yOffset);
    cr->paint();

    surface->finish();
}

ValueResult<PageImage> PageImage::create(Page &page, const Cairo::Matrix &ctm, Operator *op, ContentStream *cs) {
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
    ctm.transform_point(xOffset, yOffset);
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
