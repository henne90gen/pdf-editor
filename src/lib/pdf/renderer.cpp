#include "renderer.h"

#include <spdlog/spdlog.h>

#include "helper/util.h"
#include "operator_parser.h"

namespace pdf {

void Renderer::render() {
    // TODO set graphics state to default values
    // NOTE the ctm of cairo already translates into the correct coordinate system, this has to be preserved

    auto cropBox = page.crop_box();
    cr->set_source_rgb(1, 1, 1);
    cr->rectangle(0, 0, cropBox->width(), cropBox->height());
    cr->fill();

    traverse();
}

void Renderer::on_show_text(Operator *op) {
    const auto &textState = stateStack.back().textState;
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
