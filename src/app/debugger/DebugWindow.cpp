#include "DebugWindow.h"

#include <gtkmm/eventcontrollermotion.h>
#include <gtkmm/overlay.h>
#include <spdlog/spdlog.h>

#include "ContentContainer.h"

DebugWindow::DebugWindow(BaseObjectType *obj, const Glib::RefPtr<Gtk::Builder> &_builder, pdf::Document _document)
    : Gtk::ApplicationWindow(obj), document(std::move(_document)) {
    Gtk::Builder::get_widget_derived<ContentContainer>(_builder, "ContentContainer", document);
}
