#include "DebugApplication.h"

#include <glibmm/markup.h>
#include <gtkmm/builder.h>
#include <spdlog/spdlog.h>

#include <pdf/document.h>

#include "DebugWindow.h"
#include "debugger.xml.h"

DebugApplication::DebugApplication()
    : Gtk::Application("de.henne90gen.pdf.debugger", Gio::Application::Flags::HANDLES_OPEN) {}

void DebugApplication::on_activate() { spdlog::trace("Activating application"); }

void DebugApplication::on_open(const Gio::Application::type_vec_files &files, const Glib::ustring & /*hint*/) {
    spdlog::info("Opening {} files", files.size());

    if (files.empty()) {
        spdlog::warn("There were no files");
        return;
    }

    DebugWindow *appWindow = nullptr;
    auto windows           = get_windows();
    if (!windows.empty()) {
        appWindow = dynamic_cast<DebugWindow *>(windows[0]);
    }

    if (!appWindow) {
        // TODO only opening the first file for now -> do something with the rest of them
        try {
            auto builderString   = std::string((char *)debugger_xml_data, debugger_xml_size);
            auto builder         = Gtk::Builder::create_from_string(builderString);
            const auto &filePath = files[0]->get_path();
            appWindow            = Gtk::Builder::get_widget_derived<DebugWindow>(builder, "DebugWindow", filePath);
            add_window(*appWindow);
        } catch (Gtk::BuilderError &err) {
            spdlog::error("{}", err.what()); //
        } catch (Glib::MarkupError &err) {
            spdlog::error("{}", err.what()); //
        }
    }

    appWindow->present();
}
