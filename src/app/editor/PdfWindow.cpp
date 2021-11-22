#include "PdfWindow.h"

#include <pdf/page.h>

PdfWindow::PdfWindow(BaseObjectType *obj, const Glib::RefPtr<Gtk::Builder> &builder, pdf::Document &document)
    : Gtk::ScrolledWindow(obj) {
    pdfContainer = builder->get_widget<Gtk::Fixed>("PdfContainer");
    pdfArea      = Gtk::Builder::get_widget_derived<PdfArea>(builder, "PdfArea", document);
    get_hadjustment()->signal_value_changed().connect(sigc::mem_fun(*this, &PdfWindow::scroll_value_changed));
    get_vadjustment()->signal_value_changed().connect(sigc::mem_fun(*this, &PdfWindow::scroll_value_changed));

    double width  = 0;
    double height = PAGE_PADDING;
    document.for_each_page([&width, &height](pdf::Page *page) {
        double currentWidth = page->width();
        if (currentWidth > width) {
            width = currentWidth;
        }
        // TODO maybe add some padding between the pages
        height += page->height() + PAGE_PADDING;
        return pdf::ForEachResult::CONTINUE;
    });
    pdfContainer->set_size_request(static_cast<int>(width), static_cast<int>(height));

    auto keyCtrl = Gtk::EventControllerKey::create();
    keyCtrl->signal_key_pressed().connect(sigc::mem_fun(*this, &PdfWindow::on_key_pressed), false);
    keyCtrl->signal_key_released().connect(sigc::mem_fun(*this, &PdfWindow::on_key_released), false);
    add_controller(keyCtrl);
}

void PdfWindow::size_allocate_vfunc(int width, int height, int baseline) {
    spdlog::trace("PdfWindow::size_allocate_vfunc(width={}, height={})", width, height);
    pdfArea->set_size_request(width, height);
    Gtk::Widget::size_allocate_vfunc(width, height, baseline);
}

bool PdfWindow::on_key_pressed(guint /*keyValue*/, guint /*keyCode*/, Gdk::ModifierType /*state*/) { return false; }

void PdfWindow::on_key_released(guint /*keyValue*/, guint /*keyCode*/, Gdk::ModifierType /*state*/) {}

void PdfWindow::scroll_value_changed() {
    const auto &hadjustment = get_hadjustment();
    const auto &vadjustment = get_vadjustment();
    const auto x            = hadjustment->get_value();
    const auto y            = vadjustment->get_value();
    spdlog::trace("ContentWindow::scroll_value_changed() x={}, y={}", x, y);
    if (x == previousHAdjustment && y == previousVAdjustment) {
        return;
    }

#if 0
    if (isControlDown) {
        auto dy = y - previousVAdjustment;
        hadjustment->set_value(previousHAdjustment);
        vadjustment->set_value(previousVAdjustment);
        contentContainer->move(*contentArea, previousHAdjustment, previousVAdjustment);
        contentArea->update_zoom(dy);
        return;
    }
#endif

    pdfContainer->move(*pdfArea, x, y);
    pdfArea->set_offsets(x, y);
    previousHAdjustment = x;
    previousVAdjustment = y;
}
