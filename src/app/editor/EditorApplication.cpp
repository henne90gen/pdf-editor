#include "EditorApplication.h"

#include <glibmm/markup.h>
#include <gtkmm/builder.h>
#include <spdlog/spdlog.h>

#include <pdf/document.h>

#include "EditorWindow.h"
#include "common/OpenWindow.h"
#include "editor.xml.h"
#include "open.xml.h"

EditorApplication::EditorApplication()
    : Gtk::Application("de.henne90gen.pdf.editor", Gio::Application::Flags::HANDLES_OPEN) {}

void EditorApplication::on_activate() {
    spdlog::trace("Activating application");

    try {
        auto builderString = std::string((char *)open_xml_data, open_xml_size);
        auto builder       = Gtk::Builder::create_from_string(builderString);
        auto appWindow =
              Gtk::Builder::get_widget_derived<OpenWindow>(builder, "OpenWindow", [this](const std::string &filePath) {
                  spdlog::info("Opening file '{}'", filePath);
                  open_window(filePath);
              });
        add_window(*appWindow);
        appWindow->present();
    } catch (Gtk::BuilderError &err) {
        spdlog::error("{}", err.what()); //
    } catch (Glib::MarkupError &err) {
        spdlog::error("{}", err.what()); //
    }
}

void EditorApplication::on_open(const Gio::Application::type_vec_files &files, const Glib::ustring & /*hint*/) {
    spdlog::info("Opening {} files", files.size());

    if (files.empty()) {
        spdlog::warn("There were no files");
        return;
    }

    for (auto &file : files) {
        open_window(file->get_path());
    }
}

void EditorApplication::open_window(const std::string &filePath) {
    try {
        auto builderString = std::string((char *)editor_xml_data, editor_xml_size);
        auto builder       = Gtk::Builder::create_from_string(builderString);
        auto appWindow     = Gtk::Builder::get_widget_derived<EditorWindow>(builder, "EditorWindow", filePath);
        add_window(*appWindow);
        appWindow->present();
    } catch (Gtk::BuilderError &err) {
        spdlog::error("{}", err.what()); //
    } catch (Glib::MarkupError &err) {
        spdlog::error("{}", err.what()); //
    }
}
