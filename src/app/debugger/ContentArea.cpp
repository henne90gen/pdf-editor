#include "ContentArea.h"

#include <gtkmm/eventcontrollermotion.h>
#include <gtkmm/eventcontrollerscroll.h>
#include <gtkmm/gestureclick.h>
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

    auto gestureClick = Gtk::GestureClick::create();
    gestureClick->signal_released().connect(sigc::mem_fun(*this, &ContentArea::on_mouse_click));
    add_controller(gestureClick);

    auto scrollCtrl = Gtk::EventControllerScroll::create();
    scrollCtrl->signal_scroll().connect(sigc::mem_fun(*this, &ContentArea::on_scroll), false);
    add_controller(scrollCtrl);
}

void ContentArea::on_draw(const Cairo::RefPtr<Cairo::Context> &cr, int w, int h) const {
    spdlog::trace("ContentArea::on_draw(width={}, height={}) offsetX={} offsetY={}", w, h, offsetX, offsetY);

    cr->save();
    cr->translate(-offsetX, -offsetY);
    cr->scale(zoom, zoom);

    highlight_range(cr, document.data, document.sizeInBytes, 1, 1, 1);

    highlight_trailer(cr);
    highlight_objects(cr);

    draw_text(cr);

    highlight_byte(cr, hoveredByte, 0.2, 0.2, 0.2);
    highlight_byte(cr, selectedByte, 1.0, 0, 0);

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

void ContentArea::highlight_byte(const Cairo::RefPtr<Cairo::Context> &cr, int byte, double r, double g, double b) {
    if (byte == -1) {
        return;
    }

    cr->set_source_rgb(r, g, b);
    int x = byte % BYTES_PER_ROW;
    int y = byte / BYTES_PER_ROW;
    cr->rectangle(x * PIXELS_PER_BYTE, y * PIXELS_PER_BYTE, PIXELS_PER_BYTE, PIXELS_PER_BYTE);
    cr->stroke();
}

void ContentArea::on_mouse_leave() { signalHoveredByte.emit(-1); }

void ContentArea::on_mouse_enter(double x, double y) { update_hovered_byte(x, y); }

void ContentArea::on_mouse_motion(double x, double y) { update_hovered_byte(x, y); }

void ContentArea::on_mouse_click(int numPress, double x, double y) {
    spdlog::info("ContentArea::on_mouse_click(numPress={}, x={}, y={})", numPress, x, y);
    if (numPress != 1) {
        return;
    }

    selectedByte = find_byte(x, y);
    spdlog::trace("ContentArea::on_mouse_click(x={}, y={}) selectedByte={}", x, y, selectedByte);
    signalSelectedByte.emit(selectedByte);
    queue_draw();
}

bool ContentArea::on_scroll(double x, double y) {
    spdlog::info("ContentArea::on_scroll(x={}, y={})", x, y);
    return false;
}

int ContentArea::find_byte(double x, double y) const {
    auto canvasX = x + offsetX;
    auto canvasY = y + offsetY;
    auto byteX   = static_cast<int>(canvasX) / PIXELS_PER_BYTE;
    auto byteY   = static_cast<int>(canvasY) / PIXELS_PER_BYTE;
    return byteY * BYTES_PER_ROW + byteX;
}

void ContentArea::update_hovered_byte(double x, double y) {
    hoveredByte = find_byte(x, y);
    spdlog::trace("ContentArea::update_hovered_byte(x={}, y={}) hoveredByte={}", x, y, hoveredByte);
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

void ContentArea::update_zoom(double /*d*/) {
    zoom += 0.01;
    if (zoom < 0.1) {
        zoom = 0.1;
    } else if (zoom > 10.0) {
        zoom = 10.0;
    }
    queue_draw();
}
