#include "ContentArea.h"

#include <random>
#include <spdlog/spdlog.h>

ContentArea::ContentArea(BaseObjectType *obj, const Glib::RefPtr<Gtk::Builder> & /*builder*/, pdf::Document &_document)
    : Gtk::DrawingArea(obj), document(_document) {
    set_draw_func(sigc::mem_fun(*this, &ContentArea::on_draw));
}

void ContentArea::on_draw(const Cairo::RefPtr<Cairo::Context> &cr, int w, int h) const {
    spdlog::trace("ContentArea::on_draw(width={}, height={})", w, h);

    highlight_range(cr, document.data, document.sizeInBytes, 1, 1, 1);

    highlight_trailers(cr);
    highlight_object_starts(cr);

    draw_text(cr);
}

void ContentArea::highlight_trailers(const Cairo::RefPtr<Cairo::Context> &cr) const {
    auto *currentTrailer = &document.trailer;
    while (currentTrailer != nullptr) {
        if (currentTrailer->dict != nullptr) {
            auto start = currentTrailer->dict->data.data();
            auto size  = currentTrailer->dict->data.size();
            highlight_range(cr, start, size, 0, 0, 1);
        } else {
            auto start = currentTrailer->stream->data.data();
            auto size  = currentTrailer->stream->data.size();
            highlight_range(cr, start, size, 0, 0, 1);
        }
        currentTrailer = currentTrailer->prev;
    }
}

void ContentArea::highlight_object_starts(const Cairo::RefPtr<Cairo::Context> &cr) const {
    std::mt19937 engine; // NOLINT(cert-msc51-cpp)
    auto dist = std::uniform_real_distribution(0.0, 1.0);

    auto objects = document.objects();
    for (auto &object : objects) {
        double r = dist(engine);
        double g = dist(engine);
        double b = dist(engine);
        highlight_range(cr, object->data.data(), object->data.size(), r, g, b);
    }
}

void ContentArea::draw_text(const Cairo::RefPtr<Cairo::Context> &cr) const {
    cr->set_source_rgb(0, 0, 0);
    cr->set_font_size(PIXELS_PER_BYTE);
    cr->move_to(0, PIXELS_PER_BYTE);

    auto rowCount = static_cast<int>(document.sizeInBytes) / BYTES_PER_ROW + 1;
    for (int row = 0; row < rowCount; row++) {
        for (int col = 0; col < BYTES_PER_ROW; col++) {
            size_t byteOffset = row * BYTES_PER_ROW + col;
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

void ContentArea::highlight_range(const Cairo::RefPtr<Cairo::Context> &cr, const char *startPtr, size_t lengthIn,
                                  double r, double g, double b) const {
    cr->set_source_rgb(r, g, b);

    int startByte = static_cast<int>(startPtr - document.data);
    int length    = static_cast<int>(lengthIn);
    if (startByte % BYTES_PER_ROW != 0) {
        int x     = startByte % BYTES_PER_ROW;
        int y     = startByte / BYTES_PER_ROW;
        int width = std::min(BYTES_PER_ROW - (startByte % BYTES_PER_ROW), static_cast<int>(length));
        cr->rectangle(x * PIXELS_PER_BYTE, y * PIXELS_PER_BYTE, width * PIXELS_PER_BYTE, PIXELS_PER_BYTE);

        startByte += width;
        length -= width;
    }

    if (length >= BYTES_PER_ROW) {
        int y        = startByte / BYTES_PER_ROW;
        int width    = BYTES_PER_ROW;
        int rowCount = static_cast<int>(length) / BYTES_PER_ROW;
        cr->rectangle(0, y * PIXELS_PER_BYTE, width * PIXELS_PER_BYTE, rowCount * PIXELS_PER_BYTE);

        length -= BYTES_PER_ROW * rowCount;
        startByte += BYTES_PER_ROW * rowCount;
    }

    if (length > 0) {
        int y     = startByte / BYTES_PER_ROW;
        int width = static_cast<int>(length);
        cr->rectangle(0, y * PIXELS_PER_BYTE, width * PIXELS_PER_BYTE, PIXELS_PER_BYTE);
    }

    cr->fill();
}
