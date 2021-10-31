#pragma once

#include <gtkmm/drawingarea.h>

class ByteHighlightOverlay : public Gtk::DrawingArea {
  public:
    ByteHighlightOverlay() { set_draw_func(sigc::mem_fun(*this, &ByteHighlightOverlay::on_draw)); }

    void on_draw(const Cairo::RefPtr<Cairo::Context> &cr, int width, int height) const;
    void set_highlighted_byte(int b);

  private:
    int highlightedByte = -1;
};
