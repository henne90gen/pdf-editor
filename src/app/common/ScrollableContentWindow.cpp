#include "ScrollableContentWindow.h"

#include <gdk/gdkkeysyms.h>
#include <gtkmm/adjustment.h>
#include <gtkmm/eventcontrollerkey.h>
#include <gtkmm/eventcontrollerscroll.h>
#include <spdlog/spdlog.h>

ScrollableContentWindow::ScrollableContentWindow(BaseObjectType *obj, const Glib::RefPtr<Gtk::Builder> &builder,
                                                 const std::string &containerName)
    : Gtk::ScrolledWindow(obj) {
    contentContainer = builder->get_widget<Gtk::Fixed>(containerName);
    get_hadjustment()->signal_value_changed().connect(
          sigc::mem_fun(*this, &ScrollableContentWindow::scroll_value_changed));
    get_vadjustment()->signal_value_changed().connect(
          sigc::mem_fun(*this, &ScrollableContentWindow::scroll_value_changed));

    auto keyCtrl = Gtk::EventControllerKey::create();
    keyCtrl->signal_key_pressed().connect(sigc::mem_fun(*this, &ScrollableContentWindow::on_key_pressed), false);
    keyCtrl->signal_key_released().connect(sigc::mem_fun(*this, &ScrollableContentWindow::on_key_released), false);
    add_controller(keyCtrl);

    auto scrollCtrl = Gtk::EventControllerScroll::create();
    scrollCtrl->set_flags(Gtk::EventControllerScroll::Flags::VERTICAL);
    scrollCtrl->signal_scroll().connect(sigc::mem_fun(*this, &ScrollableContentWindow::on_scroll), false);
    add_controller(scrollCtrl);
}

void ScrollableContentWindow::size_allocate_vfunc(int width, int height, int baseline) {
    spdlog::trace("ScrollableContentWindow::size_allocate_vfunc(width={}, height={})", width, height);
    container().set_size_request(width, height);
    Gtk::Widget::size_allocate_vfunc(width, height, baseline);
}

bool ScrollableContentWindow::on_key_pressed(guint keyValue, guint keyCode, Gdk::ModifierType state) {
    if (keyValue == GDK_KEY_Control_L) {
        isControlDown = true;
    }
    return false;
}

void ScrollableContentWindow::on_key_released(guint keyValue, guint /*keyCode*/, Gdk::ModifierType /*state*/) {
    if (keyValue == GDK_KEY_Control_L) {
        isControlDown = false;
    }
}

bool ScrollableContentWindow::on_scroll(double /*x*/, double y) {
    if (isControlDown) {
        container().update_zoom(y);
        update_container_size();
        return true;
    }
    return false;
}

void ScrollableContentWindow::scroll_value_changed() {
    const auto &hadjustment = get_hadjustment();
    const auto &vadjustment = get_vadjustment();
    const auto x            = hadjustment->get_value();
    const auto y            = vadjustment->get_value();
    spdlog::trace("ScrollableContentWindow::scroll_value_changed() x={}, y={}", x, y);

    contentContainer->move(container(), x, y);
    container().set_offsets(x, y);
}

void ScrollableContentWindow::update_container_size() {
    auto size        = calculate_content_size();
    auto zoom        = container().zoom();
    auto finalWidth  = static_cast<int>(size.first * zoom);
    auto finalHeight = static_cast<int>(size.second * zoom);
    contentContainer->set_size_request(finalWidth, finalHeight);
}
