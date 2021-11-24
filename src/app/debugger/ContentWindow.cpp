#include "ContentWindow.h"

ContentWindow::ContentWindow(BaseObjectType *obj, const Glib::RefPtr<Gtk::Builder> &builder, pdf::Document &_document)
    : ScrollableContentWindow(obj, builder, "ContentContainer"), document(_document) {
    contentArea = Gtk::Builder::get_widget_derived<ContentArea>(builder, "ContentArea", document);
    contentArea->signal_selected_byte().connect(sigc::mem_fun(*this, &ContentWindow::scroll_to_byte));

    grab_focus();
    update_container_size();
}

void ContentWindow::scroll_to_byte(int byte) {
    int byteX      = byte % BYTES_PER_ROW;
    int byteY      = byte / BYTES_PER_ROW;
    double offsetX = byteX * PIXELS_PER_BYTE;
    double offsetY = byteY * PIXELS_PER_BYTE;
    get_hadjustment()->set_value(offsetX);
    get_vadjustment()->set_value(offsetY);
}

std::pair<double, double> ContentWindow::calculate_content_size() {
    int width = BYTES_PER_ROW * PIXELS_PER_BYTE;
    int numRows =
          static_cast<int>(std::ceil((static_cast<double>(document.sizeInBytes) / static_cast<double>(BYTES_PER_ROW))));
    int height = numRows * PIXELS_PER_BYTE;
    return {static_cast<double>(width), static_cast<double>(height)};
}