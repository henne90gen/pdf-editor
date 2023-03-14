#include "renderer.h"

#include <spdlog/spdlog.h>

#include "pdf/operator_parser.h"

namespace pdf {

void Renderer::render() {
    // NOTE the ctm of cairo already translates into the correct coordinate system, this has to be preserved
    cr->save();

    auto cropBox = page.crop_box();
    cr->set_source_rgb(1, 1, 1);
    cr->rectangle(0, 0, cropBox->width(), cropBox->height());
    cr->fill();

    traverse();

    cr->restore();
}

void Renderer::on_show_text(Operator *op) {
    const auto &textState = state().textState;
    cr->set_font_matrix(font_matrix());
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

    cr->show_glyphs(glyphs);
}

void Renderer::on_do(Operator *op) {
    auto pageImageResult = pdf::PageImage::create(page, state(), op, currentContentStream);
    if (pageImageResult.has_error()) {
        return;
    }

    auto pageImage           = pageImageResult.value();
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

    auto pBuf = reinterpret_cast<uint8_t *>(std::malloc(stride * height));
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
    free(pBuf);
}

} // namespace pdf
