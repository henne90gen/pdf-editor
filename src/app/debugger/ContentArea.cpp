#include "ContentArea.h"

#include <spdlog/spdlog.h>

ContentArea::ContentArea(BaseObjectType *obj, const Glib::RefPtr<Gtk::Builder> &_builder, pdf::Document &_document)
    : Gtk::DrawingArea(obj), document(_document) {
    set_draw_func(sigc::mem_fun(*this, &ContentArea::on_draw));
}

void ContentArea::on_draw(const Cairo::RefPtr<Cairo::Context> &cr, int w, int h) const {
    spdlog::trace("ContentArea::on_draw(width={}, height={})", w, h);

    int width        = BYTES_PER_ROW * PIXELS_PER_BYTE;
    int fullRowCount = static_cast<int>(document.sizeInBytes) / BYTES_PER_ROW;
    int coreHeight   = fullRowCount * PIXELS_PER_BYTE;
    cr->set_source_rgb(1, 1, 1);
    cr->rectangle(0, 0, width, coreHeight);

    int overlap = static_cast<int>(document.sizeInBytes) - (fullRowCount * BYTES_PER_ROW);
    cr->rectangle(0, coreHeight, overlap * PIXELS_PER_BYTE, PIXELS_PER_BYTE);

    //    auto *currentTrailer = &document.trailer;
    //    while (currentTrailer!= nullptr) {
    //
    //        cr->rectangle();
    //    }

    cr->fill();

    cr->set_source_rgb(0, 0, 0);
    cr->set_font_size(PIXELS_PER_BYTE);
    cr->move_to(0, PIXELS_PER_BYTE);

    for (int row = 0; row < fullRowCount + 1; row++) {
        for (int col = 0; col < BYTES_PER_ROW; col++) {
            auto byteOffset = row * BYTES_PER_ROW + col;
            if (byteOffset >= document.sizeInBytes) {
                break;
            }

            char *ptr = document.data + byteOffset;
            if (*ptr < 32 || *ptr > 126) {
                continue;
            }

            int x             = col * PIXELS_PER_BYTE;
            int y             = (row + 1) * PIXELS_PER_BYTE;
            auto str          = std::string(ptr, 1);
            auto glyphs       = std::vector<Cairo::Glyph>(1);
            auto clusters     = std::vector<Cairo::TextCluster>(1);
            auto clusterFlags = Cairo::TextClusterFlags();
            cr->get_scaled_font()->text_to_glyphs(x, y, str, glyphs, clusters, clusterFlags);
            cr->show_text_glyphs(str, glyphs, clusters, clusterFlags);
        }
    }
}
