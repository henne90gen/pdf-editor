#pragma once

// TODO this is a hack to get the gtkmm4 code to compile on Windows
#undef WIN32
#include <gtkmm/builder.h>
#include <gtkmm/drawingarea.h>

#include <pdf/document.h>

#include "ScrollableContentWindow.h"

constexpr int PIXELS_PER_BYTE = 50;
constexpr int BYTES_PER_ROW   = 50;

class ContentArea : public ScrolledContainer {
  public:
    double _zoom = 1.0;

    [[maybe_unused]] ContentArea(BaseObjectType *obj, const Glib::RefPtr<Gtk::Builder> &_builder,
                                 pdf::Document &_document);

    using type_signal_byte = sigc::signal<void(int)>;
    type_signal_byte signal_hovered_byte() { return signalHoveredByte; }
    type_signal_byte signal_selected_byte() { return signalSelectedByte; }

    void toggle_highlight_trailer();
    void toggle_highlight_objects();

    void set_offsets(double x, double y) override;
    void update_zoom(double z) override;
    double zoom() override { return _zoom; }

    void set_selected_byte(int byte) {
        selectedByte = byte;
        signalSelectedByte.emit(selectedByte);
    }

  protected:
    void on_draw(const Cairo::RefPtr<Cairo::Context> &cr, int width, int height) const;
    void on_mouse_leave();
    void on_mouse_enter(double x, double y);
    void on_mouse_motion(double x, double y);
    void on_mouse_click(int numPress, double x, double y);

  private:
    pdf::Document &document;
    double offsetX              = 0.0;
    double offsetY              = 0.0;
    bool shouldHighlightTrailer = false;
    bool shouldHighlightObjects = false;
    int hoveredByte             = -1;
    int selectedByte            = -1;

    type_signal_byte signalHoveredByte;
    type_signal_byte signalSelectedByte;

    void draw_text(const Cairo::RefPtr<Cairo::Context> &cr) const;
    void highlight_trailer(const Cairo::RefPtr<Cairo::Context> &cr) const;
    void highlight_objects(const Cairo::RefPtr<Cairo::Context> &cr) const;
    void update_hovered_byte(double x, double y);

    int find_byte(double x, double y) const;
    void highlight_range(const Cairo::RefPtr<Cairo::Context> &cr, const char *startPtr, size_t length, double r,
                         double g, double b) const;
    static void highlight_byte(const Cairo::RefPtr<Cairo::Context> &cr, int byte, double r, double g, double b);
};
