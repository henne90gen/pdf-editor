#include "DebugApplication.h"

#include <glibmm/markup.h>
#include <gtkmm/builder.h>
#include <spdlog/spdlog.h>

#include <pdf/document.h>

#include "DebugWindow.h"
#include "debugger.xml.h"

DebugApplication::DebugApplication() : PdfApplication("de.henne90gen.pdf.debugger") {}

void DebugApplication::open_window(const std::string &filePath) {
    try {
        auto builderString = std::string((char *)debugger_xml_data, debugger_xml_size);
        auto builder       = Gtk::Builder::create_from_string(builderString);
        auto window        = Gtk::Builder::get_widget_derived<DebugWindow>(builder, "DebugWindow", filePath);
        add_window(*window);
        window->present();
    } catch (Gtk::BuilderError &err) {
        spdlog::error("{}", err.what()); //
    } catch (Glib::MarkupError &err) {
        spdlog::error("{}", err.what()); //
    }
}
