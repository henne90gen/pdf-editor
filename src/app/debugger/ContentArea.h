#pragma once

// TODO this is a hack to get the gtkmm4 code to compile on Windows
#undef WIN32
#include <gtkmm/builder.h>
#include <gtkmm/drawingarea.h>

#include <pdf/document.h>

constexpr int PIXELS_PER_BYTE = 50;
constexpr int BYTES_PER_ROW   = 50;

class ContentArea : public Gtk::DrawingArea {
  public:
    [[maybe_unused]] ContentArea(BaseObjectType *obj, const Glib::RefPtr<Gtk::Builder> &_builder,
                                 pdf::Document &_document);

    void on_draw(const Cairo::RefPtr<Cairo::Context> &cr, int width, int height) const;

    void toggle_highlight_trailer() {
        shouldHighlightTrailer = !shouldHighlightTrailer;
        queue_draw();
    }
    void toggle_highlight_objects() {
        shouldHighlightObjects = !shouldHighlightObjects;
        queue_draw();
    }

    void resize_and_set_offsets(int width, int height, double x, double y) {
        offsetX = x;
        offsetY = y;
        set_size_request(width, height);
    }

  private:
    void draw_text(const Cairo::RefPtr<Cairo::Context> &cr) const;
    void highlight_trailer(const Cairo::RefPtr<Cairo::Context> &cr) const;
    void highlight_objects(const Cairo::RefPtr<Cairo::Context> &cr) const;
    void highlight_range(const Cairo::RefPtr<Cairo::Context> &cr, const char *startPtr, size_t length, double r,
                         double g, double b) const;

    pdf::Document &document;
    double offsetX = 0.0;
    double offsetY = 0.0;
    bool shouldHighlightTrailer = false;
    bool shouldHighlightObjects = false;
};
