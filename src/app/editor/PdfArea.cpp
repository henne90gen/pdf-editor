#include "PdfArea.h"

#include <pdf/renderer.h>
#include <spdlog/spdlog.h>

PdfArea::PdfArea(BaseObjectType *obj, const Glib::RefPtr<Gtk::Builder> &, pdf::Document &_document)
    : Gtk::DrawingArea(obj), document(_document) {
    set_draw_func(sigc::mem_fun(*this, &PdfArea::on_draw));
}

void PdfArea::on_draw(const Cairo::RefPtr<Cairo::Context> &cr, int width, int height) {
    spdlog::trace("PdfArea::on_draw(width={}, height={})", width, height);

    cr->translate(-offsetX, -offsetY);
    //    cr->scale(zoom, zoom);

    auto pages = document.pages();
    for (auto page : pages) {
        pdf::Renderer renderer(*page);
        renderer.render(cr);

        cr->translate(0, page->height() + PAGE_PADDING);
    }
}

void PdfArea::set_offsets(const double x, const double y) {
    offsetX = x;
    offsetY = y;
    queue_draw();
}
