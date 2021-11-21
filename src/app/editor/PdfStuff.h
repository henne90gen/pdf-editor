#pragma once

#include <iostream>
#include <spdlog/spdlog.h>

// TODO this is a hack to get the gtkmm4 code to compile on Windows
#undef WIN32
#include <glibmm/convert.h>
#include <glibmm/markup.h>
#include <gtkmm/box.h>
#include <gtkmm/builder.h>
#include <gtkmm/drawingarea.h>
#include <gtkmm/fixed.h>
#include <gtkmm/frame.h>
#include <gtkmm/label.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/treestore.h>
#include <gtkmm/treeview.h>
#include <gtkmm/viewport.h>

#include <pdf/document.h>

#if 0
class PdfWidget : public Gtk::Viewport {
  public:
    explicit PdfWidget(pdf::Document &file);

    void on_draw(const Cairo::RefPtr<Cairo::Context> &cr, int width, int height);
    //    bool on_scroll_event(GdkEventScroll *event) override;
    //    bool on_button_press_event(GdkEventButton *button_event) override;
    //    bool on_key_press_event(GdkEventKey *key_event) override;
    //    bool on_key_release_event(GdkEventKey *key_event) override;

  protected:
    void size_allocate_vfunc(int w, int h, int baseline) override;
    void update_adjustments(int w, int h);

  private:
    pdf::Document &file;
    Gtk::DrawingArea drawingArea;

    [[maybe_unused]] bool isCtrlPressed = false;
    double zoom                         = 1.0;
    [[maybe_unused]] double zoomSpeed   = 0.1;
};
#endif

class PdfWindow : public Gtk::ScrolledWindow {
  public:
    [[maybe_unused]] PdfWindow(BaseObjectType *obj, const Glib::RefPtr<Gtk::Builder> & /*builder*/)
        : Gtk::ScrolledWindow(obj) {}
};

class PdfArea : public Gtk::DrawingArea {
  public:
    [[maybe_unused]] PdfArea(BaseObjectType *obj, const Glib::RefPtr<Gtk::Builder> & /*builder*/)
        : Gtk::DrawingArea(obj) {}
};
