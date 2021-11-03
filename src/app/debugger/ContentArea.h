#pragma once

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

  private:
    void draw_text(const Cairo::RefPtr<Cairo::Context> &cr) const;
    void highlight_trailer(const Cairo::RefPtr<Cairo::Context> &cr) const;
    void highlight_objects(const Cairo::RefPtr<Cairo::Context> &cr) const;
    void highlight_range(const Cairo::RefPtr<Cairo::Context> &cr, const char *startPtr, size_t length, double r,
                         double g, double b) const;

    pdf::Document &document;
    bool shouldHighlightTrailer = false;
    bool shouldHighlightObjects = false;
};
