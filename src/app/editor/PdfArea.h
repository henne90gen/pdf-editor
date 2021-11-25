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
#include <pdf/page.h>

#include <ScrolledZoomedWindow.h>

constexpr int PAGE_PADDING = 10;

class PdfArea : public ScrolledZoomedContent {
  public:
    double _zoom = 1.0;

    [[maybe_unused]] PdfArea(BaseObjectType *obj, const Glib::RefPtr<Gtk::Builder> & /*builder*/,
                             pdf::Document &_document);

    void set_offsets(double x, double y) override;
    void update_zoom(double z) override;
    double zoom() override { return _zoom; }

    void on_draw(const Cairo::RefPtr<Cairo::Context> &cr, int width, int height);
    void on_highlight_text_toggled();

  private:
    pdf::Document &document;

    double offsetX = 0.0;
    double offsetY = 0.0;

    bool highlightText                     = false;
    std::vector<pdf::TextBlock> textBlocks = {};

    void render_pages(const Cairo::RefPtr<Cairo::Context> &cr);
    void render_text_blocks(const Cairo::RefPtr<Cairo::Context> &cr);
};
