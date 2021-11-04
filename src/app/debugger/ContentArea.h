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
    void on_mouse_leave();
    void on_mouse_enter(double x, double y);
    void on_mouse_motion(double x, double y);

    using type_signal_selected_byte = sigc::signal<void(int)>;
    type_signal_selected_byte signal_hovered_byte() { return signalHoveredByte; }

    void toggle_highlight_trailer();
    void toggle_highlight_objects();

    void set_offsets(double x, double y);

  private:
    void draw_text(const Cairo::RefPtr<Cairo::Context> &cr) const;
    void highlight_trailer(const Cairo::RefPtr<Cairo::Context> &cr) const;
    void highlight_objects(const Cairo::RefPtr<Cairo::Context> &cr) const;
    void highlight_range(const Cairo::RefPtr<Cairo::Context> &cr, const char *startPtr, size_t length, double r,
                         double g, double b) const;
    void highlight_hovered_byte(const Cairo::RefPtr<Cairo::Context> &cr) const;
    void update_highlighted_byte(double x, double y);

    pdf::Document &document;
    double offsetX              = 0.0;
    double offsetY              = 0.0;
    bool shouldHighlightTrailer = false;
    bool shouldHighlightObjects = false;
    int hoveredByte             = -1;

    type_signal_selected_byte signalHoveredByte;
};
