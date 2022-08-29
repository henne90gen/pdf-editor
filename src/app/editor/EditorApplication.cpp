#include "EditorApplication.h"

#include <glibmm/markup.h>
#include <gtkmm/builder.h>
#include <spdlog/spdlog.h>

#include "EditorWindow.h"

EditorApplication::EditorApplication() : PdfApplication("de.henne90gen.pdf.editor") {
    if (!register_application()) {
        spdlog::warn("failed to register application");
        return;
    }

    auto action = add_action("save");
    action->signal_activate().connect(sigc::mem_fun(*this, &EditorApplication::on_save));

    auto fileMenu = Gio::Menu::create();
    auto saveItem = Gio::MenuItem::create("Save", "app.save");
    fileMenu->append_item(saveItem);

    auto menu = Gio::Menu::create();
    menu->append_submenu("File", fileMenu);
    set_menubar(menu);
}

void EditorApplication::open_window(const std::string &filePath) {
    try {
        auto builder = Gtk::Builder::create_from_resource("/com/github/henne90gen/pdf-editor/editor.ui");
        auto window  = Gtk::Builder::get_widget_derived<EditorWindow>(builder, "EditorWindow", filePath);
        add_window(*window);
        window->present();
    } catch (Gtk::BuilderError &err) {
        spdlog::error("{}", err.what()); //
    } catch (Glib::MarkupError &err) {
        spdlog::error("{}", err.what()); //
    }
}

void EditorApplication::on_save(const Glib::VariantBase & /*parameter*/) {
    auto activeWindow = get_active_window();
    if (activeWindow != nullptr) {
        reinterpret_cast<EditorWindow*>(activeWindow)->save();
    }
}
