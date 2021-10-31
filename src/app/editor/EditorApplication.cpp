#include "EditorApplication.h"

#include <glibmm/markup.h>
#include <gtkmm/builder.h>
#include <spdlog/spdlog.h>

#include <pdf/document.h>

#include "EditorWindow.h"
#include "editor.glade.h"

EditorApplication::EditorApplication()
    : Gtk::Application("de.henne90gen.pdf.editor", Gio::Application::Flags::HANDLES_OPEN) {}

void EditorApplication::on_activate() { spdlog::trace("Activating application"); }

void EditorApplication::on_open(const Gio::Application::type_vec_files &files, const Glib::ustring & /*hint*/) {
    spdlog::info("Opening files");

    if (files.empty()) {
        spdlog::warn("There were no files");
        return;
    }

    EditorWindow *appWindow = nullptr;
    auto windows            = get_windows();
    if (!windows.empty()) {
        appWindow = dynamic_cast<EditorWindow *>(windows[0]);
    }

    if (!appWindow) {
        try {
            auto builderString = std::string((char *)editor_glade_data, editor_glade_size);
            auto builder       = Gtk::Builder::create_from_string(builderString);
            appWindow          = Gtk::Builder::get_widget_derived<EditorWindow>(builder, "EditorWindow");
            add_window(*appWindow);
        } catch (Gtk::BuilderError &err) {
            spdlog::error("{}", err.what()); //
        } catch (Glib::MarkupError &err) {
            spdlog::error("{}", err.what()); //
        }
    }

    for (auto &file : files) {
        appWindow->add_document(file->get_path());
    }

    appWindow->present();
}
