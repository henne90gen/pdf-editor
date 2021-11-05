#include "ContentArea.h"

#include <gtkmm/eventcontrollermotion.h>
#include <random>
#include <spdlog/spdlog.h>

ContentArea::ContentArea(BaseObjectType *obj, const Glib::RefPtr<Gtk::Builder> & /*builder*/, pdf::Document &_document)
    : Gtk::DrawingArea(obj), document(_document) {
    set_draw_func(sigc::mem_fun(*this, &ContentArea::on_draw));

    auto motionCtrl = Gtk::EventControllerMotion::create();
    motionCtrl->signal_leave().connect(sigc::mem_fun(*this, &ContentArea::on_mouse_leave));
    motionCtrl->signal_enter().connect(sigc::mem_fun(*this, &ContentArea::on_mouse_enter));
    motionCtrl->signal_motion().connect(sigc::mem_fun(*this, &ContentArea::on_mouse_motion));
    add_controller(motionCtrl);
}

void ContentArea::on_draw(const Cairo::RefPtr<Cairo::Context> &cr, int w, int h) const {
    spdlog::trace("ContentArea::on_draw(width={}, height={}) offsetX={} offsetY={}", w, h, offsetX, offsetY);

    cr->save();
    cr->translate(-offsetX, -offsetY);

    highlight_range(cr, document.data, document.sizeInBytes, 1, 1, 1);

    highlight_trailer(cr);
    highlight_objects(cr);

    draw_text(cr);

    highlight_hovered_byte(cr);

    cr->restore();
}

void ContentArea::highlight_trailer(const Cairo::RefPtr<Cairo::Context> &cr) const {
    if (!shouldHighlightTrailer) {
        return;
    }

    // TODO maybe skip highlighting trailers that are currently not visible
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

void ContentArea::highlight_objects(const Cairo::RefPtr<Cairo::Context> &cr) const {
    if (!shouldHighlightObjects) {
        return;
    }

    auto engine = std::mt19937(1337); // NOLINT(cert-msc51-cpp)
    auto dist   = std::uniform_real_distribution(0.0, 1.0);

    // TODO this might be slow for large files -> don't highlight objects that are not visible
    document.for_each_object([&](pdf::IndirectObject *object) {
        double r = dist(engine);
        double g = dist(engine);
        double b = dist(engine);
        highlight_range(cr, object->data.data(), object->data.size(), r, g, b);
        return true;
    });
}

void ContentArea::draw_text(const Cairo::RefPtr<Cairo::Context> &cr) const {
    cr->set_source_rgb(0, 0, 0);
    cr->set_font_size(PIXELS_PER_BYTE);
    cr->move_to(0, PIXELS_PER_BYTE);

    // TODO only go over the rows that will actually show up in the final output
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

void ContentArea::highlight_hovered_byte(const Cairo::RefPtr<Cairo::Context> &cr) const {
    if (hoveredByte == -1) {
        return;
    }

    cr->set_source_rgb(1, 0, 0);
    int x = hoveredByte % BYTES_PER_ROW;
    int y = hoveredByte / BYTES_PER_ROW;
    cr->rectangle(x * PIXELS_PER_BYTE, y * PIXELS_PER_BYTE, PIXELS_PER_BYTE, PIXELS_PER_BYTE);
    cr->stroke();
}

void ContentArea::on_mouse_leave() { signalHoveredByte.emit(-1); }

void ContentArea::on_mouse_enter(double x, double y) { update_highlighted_byte(x, y); }

void ContentArea::on_mouse_motion(double x, double y) { update_highlighted_byte(x, y); }

void ContentArea::update_highlighted_byte(double x, double y) {
    auto canvasX = x + offsetX;
    auto canvasY = y + offsetY;
    auto byteX   = static_cast<int>(canvasX) / PIXELS_PER_BYTE;
    auto byteY   = static_cast<int>(canvasY) / PIXELS_PER_BYTE;
    hoveredByte  = byteY * BYTES_PER_ROW + byteX;

    spdlog::trace("ContentArea::update_highlighted_byte(x={}, y={}) hoveredByte={}", x, y, hoveredByte);
    signalHoveredByte.emit(hoveredByte);
    queue_draw();
}

void ContentArea::toggle_highlight_trailer() {
    shouldHighlightTrailer = !shouldHighlightTrailer;
    queue_draw();
}

void ContentArea::toggle_highlight_objects() {
    shouldHighlightObjects = !shouldHighlightObjects;
    queue_draw();
}

void ContentArea::set_offsets(double x, double y) {
    offsetX = x;
    offsetY = y;
    queue_draw();
}
