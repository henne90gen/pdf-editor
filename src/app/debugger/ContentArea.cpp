#include "ContentArea.h"

#include <spdlog/spdlog.h>

ContentArea::ContentArea(BaseObjectType *obj, const Glib::RefPtr<Gtk::Builder> &_builder, pdf::Document &_document)
    : Gtk::DrawingArea(obj), document(_document) {
    set_draw_func(sigc::mem_fun(*this, &ContentArea::on_draw));
}

void ContentArea::on_draw(const Cairo::RefPtr<Cairo::Context> &cr, int width, int height) {
    spdlog::trace("drawing: {}x{}", width, height);

    cr->set_source_rgb(1, 0, 0);
    cr->rectangle(0, 0, width / 2.0, height / 2.0);
    cr->fill();
}
