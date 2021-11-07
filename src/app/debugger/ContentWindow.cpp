#include "ContentWindow.h"

#include <gtkmm/eventcontrollerkey.h>
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

    auto keyCtrl = Gtk::EventControllerKey::create();
    keyCtrl->signal_key_pressed().connect(sigc::mem_fun(*this, &ContentWindow::on_key_pressed), false);
    keyCtrl->signal_key_released().connect(sigc::mem_fun(*this, &ContentWindow::on_key_released), false);
    add_controller(keyCtrl);
}

void ContentWindow::size_allocate_vfunc(int width, int height, int baseline) {
    spdlog::trace("ContentWindow::size_allocate_vfunc(width={}, height={})", width, height);
    contentArea->set_size_request(width, height);
    Gtk::Widget::size_allocate_vfunc(width, height, baseline);
}

void ContentWindow::scroll_value_changed() {
    const auto &hadjustment = get_hadjustment();
    const auto &vadjustment = get_vadjustment();
    const auto x            = hadjustment->get_value();
    const auto y            = vadjustment->get_value();
    spdlog::trace("ContentWindow::scroll_value_changed() x={}, y={}", x, y);
    if (x == previousHAdjustment && y == previousVAdjustment) {
        return;
    }

    if (isControlDown) {
        auto dy = y - previousVAdjustment;
        hadjustment->set_value(previousHAdjustment);
        vadjustment->set_value(previousVAdjustment);
        contentContainer->move(*contentArea, previousHAdjustment, previousVAdjustment);
        contentArea->update_zoom(dy);
        return;
    }

    contentContainer->move(*contentArea, x, y);
    contentArea->set_offsets(x, y);
    previousHAdjustment = x;
    previousVAdjustment = y;
}

void ContentWindow::scroll_to_byte(int byte) {
    int byteX      = byte % BYTES_PER_ROW;
    int byteY      = byte / BYTES_PER_ROW;
    double offsetX = byteX * PIXELS_PER_BYTE;
    double offsetY = byteY * PIXELS_PER_BYTE;
    get_hadjustment()->set_value(offsetX);
    get_vadjustment()->set_value(offsetY);
}

bool ContentWindow::on_key_pressed(guint keyValue, guint /*keyCode*/, Gdk::ModifierType /*state*/) {
    if (keyValue == GDK_KEY_Control_L) {
        isControlDown = true;
        spdlog::trace("ContentWindow::on_key_pressed(): Control down");
    }
    return false;
}

void ContentWindow::on_key_released(guint keyValue, guint /*keyCode*/, Gdk::ModifierType /*state*/) {
    if (keyValue == GDK_KEY_Control_L) {
        isControlDown = false;
        spdlog::trace("ContentWindow::on_key_released(): Control up");
    }
}
