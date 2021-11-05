#include "ContentWindow.h"

#include <spdlog/spdlog.h>

ContentWindow::ContentWindow(BaseObjectType *obj, const Glib::RefPtr<Gtk::Builder> &builder, pdf::Document &document)
    : Gtk::ScrolledWindow(obj) {
    contentContainer = builder->get_widget<Gtk::Fixed>("ContentContainer");
    contentArea      = Gtk::Builder::get_widget_derived<ContentArea>(builder, "ContentArea", document);
    get_hadjustment()->signal_value_changed().connect(sigc::mem_fun(*this, &ContentWindow::scroll_value_changed));
    get_vadjustment()->signal_value_changed().connect(sigc::mem_fun(*this, &ContentWindow::scroll_value_changed));
    contentArea->signal_selected_byte().connect(sigc::mem_fun(*this, &ContentWindow::scroll_to_byte));

    int width = BYTES_PER_ROW * PIXELS_PER_BYTE;
    int numRows =
          static_cast<int>(std::ceil((static_cast<double>(document.sizeInBytes) / static_cast<double>(BYTES_PER_ROW))));
    int height = numRows * PIXELS_PER_BYTE;
    contentContainer->set_size_request(width, height);
}

void ContentWindow::size_allocate_vfunc(int width, int height, int baseline) {
    spdlog::trace("ContentWindow::size_allocate_vfunc(width={}, height={})", width, height);
    contentArea->set_size_request(width, height);
    Gtk::Widget::size_allocate_vfunc(width, height, baseline);
}

void ContentWindow::scroll_value_changed() {
    double x = get_hadjustment()->get_value();
    double y = get_vadjustment()->get_value();
    spdlog::trace("ContentWindow::scroll_value_changed() x={}, y={}", x, y);
    contentContainer->move(*contentArea, x, y);
    contentArea->set_offsets(x, y);
}

void ContentWindow::scroll_to_byte(int byte) {
    int byteX      = byte % BYTES_PER_ROW;
    int byteY      = byte / BYTES_PER_ROW;
    double offsetX = byteX * PIXELS_PER_BYTE;
    double offsetY = byteY * PIXELS_PER_BYTE;
    get_hadjustment()->set_value(offsetX);
    get_vadjustment()->set_value(offsetY);
}
