#include "DebugWindow.h"

#include <spdlog/spdlog.h>

DebugWindow::DebugWindow(BaseObjectType *obj, const Glib::RefPtr<Gtk::Builder> &_builder, pdf::Document _document)
    : Gtk::ApplicationWindow(obj), document(std::move(_document)) {
    contentArea = Gtk::Builder::get_widget_derived<ContentArea>(_builder, "ContentArea", document);
    contentContainer =
          Gtk::Builder::get_widget_derived<ContentContainer>(_builder, "ContentContainer", contentArea, document);
}

ContentContainer::ContentContainer(BaseObjectType *obj, const Glib::RefPtr<Gtk::Builder> &_builder,
                                   Gtk::DrawingArea *_contentArea, pdf::Document &_document)
    : Gtk::Viewport(obj), contentArea(_contentArea), document(_document) {}

void ContentContainer::size_allocate_vfunc(int w, int h, int baseline) {
    spdlog::trace("size_allocate_vfunc: width={}, height={}, baseline={}", w, h, baseline);
    spdlog::info("Received document of size: {}", document.sizeInBytes);

    int width       = 500;
    int sizeInBytes = static_cast<int>(document.sizeInBytes);
    int height      = (sizeInBytes / 50) * 10;

    spdlog::trace("calculated width: {}, calculated height: {}", width, height);

    const auto &hadjustment = get_hadjustment();
    const auto &vadjustment = get_vadjustment();
    hadjustment->configure(hadjustment->get_value(), 0.0, width, 1.0, 10.0, 0.0);
    vadjustment->configure(vadjustment->get_value(), 0.0, height, 1.0, 10.0, 0.0);

    contentArea->set_size_request(width, height);

    Widget::size_allocate_vfunc(w, h, baseline);
}
