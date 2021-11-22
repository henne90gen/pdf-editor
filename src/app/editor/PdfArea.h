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

constexpr int PAGE_PADDING = 10;

class PdfArea : public Gtk::DrawingArea {
  public:
    [[maybe_unused]] PdfArea(BaseObjectType *obj, const Glib::RefPtr<Gtk::Builder> & /*builder*/,
                             pdf::Document &_document);

    void on_draw(const Cairo::RefPtr<Cairo::Context> &cr, int width, int height);
    void set_offsets(double x, double y);

  private:
    pdf::Document &document;
    double offsetX = 0.0;
    double offsetY = 0.0;
};
