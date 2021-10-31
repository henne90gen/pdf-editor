#include "ContentContainer.h"

#include <gtkmm/eventcontrollermotion.h>
#include <gtkmm/overlay.h>
#include <spdlog/spdlog.h>

ContentContainer::ContentContainer(BaseObjectType *obj, const Glib::RefPtr<Gtk::Builder> &_builder,
                                   pdf::Document &_document)
    : Gtk::Viewport(obj), document(_document) {
    auto contentOverlayContainer = _builder->get_widget<Gtk::Overlay>("ContentOverlay");
    contentOverlayContainer->add_overlay(byteHighlightOverlay);
    signal_selected_byte().connect(sigc::mem_fun(byteHighlightOverlay, &ByteHighlightOverlay::set_highlighted_byte));

    contentArea = Gtk::Builder::get_widget_derived<ContentArea>(_builder, "ContentArea", document);

    auto motionCtrl = Gtk::EventControllerMotion::create();
    motionCtrl->signal_leave().connect(sigc::mem_fun(*this, &ContentContainer::on_mouse_leave));
    motionCtrl->signal_enter().connect(sigc::mem_fun(*this, &ContentContainer::on_mouse_enter));
    motionCtrl->signal_motion().connect(sigc::mem_fun(*this, &ContentContainer::on_mouse_motion));
    add_controller(motionCtrl);
}

void ContentContainer::size_allocate_vfunc(int w, int h, int baseline) {
    int width = BYTES_PER_ROW * PIXELS_PER_BYTE;
    int numRows =
          static_cast<int>(std::ceil((static_cast<double>(document.sizeInBytes) / static_cast<double>(BYTES_PER_ROW))));
    int height = numRows * PIXELS_PER_BYTE;

    const auto &hadjustment = get_hadjustment();
    const auto &vadjustment = get_vadjustment();
    hadjustment->configure(hadjustment->get_value(), 0.0, width, 1.0, 10.0, 0.0);
    vadjustment->configure(vadjustment->get_value(), 0.0, height, 1.0, 10.0, 0.0);

    contentArea->set_size_request(width, height);

    Widget::size_allocate_vfunc(w, h, baseline);
}

void ContentContainer::on_mouse_leave() { signalSelectedByte.emit(-1); }

void ContentContainer::on_mouse_enter(double x, double y) { updateHighlightedByte(x, y); }

void ContentContainer::on_mouse_motion(double x, double y) { updateHighlightedByte(x, y); }

void ContentContainer::updateHighlightedByte(double x, double y) {
    auto hadjustment_value = get_hadjustment()->get_value();
    auto vadjustment_value = get_vadjustment()->get_value();

    auto canvasX = x + hadjustment_value;
    auto canvasY = y + vadjustment_value;
    auto byteX   = static_cast<int>(canvasX) / PIXELS_PER_BYTE;
    auto byteY   = static_cast<int>(canvasY) / PIXELS_PER_BYTE;

    auto highlightedByte = byteY * BYTES_PER_ROW + byteX;

    spdlog::trace("ContentContainer::updateHighlightedByte(x={}, y={}): highlightedByte={}", x, y, highlightedByte);
    signalSelectedByte.emit(highlightedByte);
}
