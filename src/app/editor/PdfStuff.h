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
#include <gtkmm/eventcontrollerkey.h>
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

class PdfArea : public Gtk::DrawingArea {
  public:
    [[maybe_unused]] PdfArea(BaseObjectType *obj, const Glib::RefPtr<Gtk::Builder> & /*builder*/);

    void on_draw(const Glib::RefPtr<Cairo::Context> &cr, int width, int height);
    void set_offsets(double x, double y);

  private:
    double offsetX = 0.0;
    double offsetY = 0.0;
};

class PdfWindow : public Gtk::ScrolledWindow {
  public:
    [[maybe_unused]] PdfWindow(BaseObjectType *obj, const Glib::RefPtr<Gtk::Builder> &builder);

  protected:
    void size_allocate_vfunc(int width, int height, int baseline) override;
    bool on_key_pressed(guint keyValue, guint keyCode, Gdk::ModifierType state);
    void on_key_released(guint keyValue, guint keyCode, Gdk::ModifierType state);

  private:
    Gtk::Fixed *pdfContainer;
    PdfArea *pdfArea;
    double previousHAdjustment = 0.0;
    double previousVAdjustment = 0.0;

    void scroll_value_changed();
};
