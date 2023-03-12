#pragma once

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
#include <iostream>
#include <spdlog/spdlog.h>

#include <pdf/document.h>
#include <pdf/page.h>

#include <ScrolledZoomedWindow.h>

constexpr int PAGE_PADDING = 10;

struct PageTextBlocks {
    double pageOffset                      = 0.0;
    std::vector<pdf::TextBlock> textBlocks = {};

    PageTextBlocks(double _pageOffset, std::vector<pdf::TextBlock> _textBlocks)
        : pageOffset(_pageOffset), textBlocks(std::move(_textBlocks)) {}
};

struct PageImages {
    pdf::Page *page;
    std::vector<pdf::PageImage> images = {};
};

class PdfArea : public ScrolledZoomedContent {
  public:
    double _zoom = 1.0;

    [[maybe_unused]] PdfArea(BaseObjectType *obj, const Glib::RefPtr<Gtk::Builder> & /*builder*/,
                             pdf::Document &_document);

    void set_offsets(double x, double y) override;
    void update_zoom(double z) override;
    double zoom() override { return _zoom; }

    void on_draw(const Cairo::RefPtr<Cairo::Context> &cr, int width, int height);

  private:
    pdf::Document &document;

    double scrollOffsetX = 0.0;
    double scrollOffsetY = 0.0;
    double mouseX        = 0.0;
    double mouseY        = 0.0;
    double dragStartX    = 0.0;
    double dragStartY    = 0.0;

    pdf::PageImage *selectedImage = nullptr;

    std::vector<PageTextBlocks> pageTextBlocks = {};
    std::vector<PageImages> pageImages         = {};

    void on_mouse_moved(double x, double y);
    void on_mouse_drag_begin(double x, double y);
    void on_mouse_drag_update(double x, double y);
    void on_mouse_drag_end(double x, double y);

    void render_pages(const Cairo::RefPtr<Cairo::Context> &cr);
    void render_text_highlight(const Cairo::RefPtr<Cairo::Context> &cr);
    void render_image_highlight(const Cairo::RefPtr<Cairo::Context> &cr);

    pdf::PageImage *get_image_at_position(double x, double y);
};
