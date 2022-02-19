#include "DebugApplication.h"

#include <glibmm/markup.h>
#include <gtkmm/builder.h>
#include <spdlog/spdlog.h>

#include "DebugWindow.h"
#include "debugger.xml.h"

DebugApplication::DebugApplication() : PdfApplication("de.henne90gen.pdf.debugger") {}

void DebugApplication::open_window(const std::string &filePath) {
    try {
        auto builder       = Gtk::Builder::create_from_string(embedded::get_debugger_xml());
        auto window        = Gtk::Builder::get_widget_derived<DebugWindow>(builder, "DebugWindow", filePath);
        add_window(*window);
        window->present();
    } catch (Gtk::BuilderError &err) {
        spdlog::error("{}", err.what()); //
    } catch (Glib::MarkupError &err) {
        spdlog::error("{}", err.what()); //
    }
}
