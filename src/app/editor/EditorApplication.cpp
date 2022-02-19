#include "EditorApplication.h"

#include <glibmm/markup.h>
#include <gtkmm/builder.h>
#include <spdlog/spdlog.h>

#include "EditorWindow.h"
#include "editor.xml.h"

EditorApplication::EditorApplication() : PdfApplication("de.henne90gen.pdf.editor") {}

void EditorApplication::open_window(const std::string &filePath) {
    try {
        auto builder       = Gtk::Builder::create_from_string(embedded::get_editor_xml());
        auto window        = Gtk::Builder::get_widget_derived<EditorWindow>(builder, "EditorWindow", filePath);
        add_window(*window);
        window->present();
    } catch (Gtk::BuilderError &err) {
        spdlog::error("{}", err.what()); //
    } catch (Glib::MarkupError &err) {
        spdlog::error("{}", err.what()); //
    }
}
