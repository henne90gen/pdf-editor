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

    add_action_with_parameter("open", Glib::VARIANT_TYPE_STRING,
                              sigc::mem_fun(*this, &EditorApplication::on_open_file));
    add_action("save", sigc::mem_fun(*this, &EditorApplication::on_save));
    add_action("open_file_dialog", sigc::mem_fun(*this, &EditorApplication::on_open_file_dialog));

    auto fileMenu = Gio::Menu::create();

    auto openItem = Gio::MenuItem::create("Open", "app.open_file_dialog");
    fileMenu->append_item(openItem);

    auto saveItem = Gio::MenuItem::create("Save", "app.save");
    fileMenu->append_item(saveItem);

    auto menu = Gio::Menu::create();
    menu->append_submenu("File", fileMenu);
    set_menubar(menu);
}

void EditorApplication::open_window(const std::string &filePath) {
    spdlog::info("Opening new window: {}", filePath);
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

void EditorApplication::on_open_file(const Glib::VariantBase &parameter) {
    if (!parameter.get_type().equal(Glib::VARIANT_TYPE_STRING)) {
        return;
    }

    auto path = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(parameter);
    auto pathString = locale_from_utf8(path.get());
    open_window(pathString);
}

void EditorApplication::on_open_file_dialog() {
    // TODO Migrate this to GtkFileDialog when upgrading to gtk-4.10
    dialog = Gtk::FileChooserNative::create("Open Document", Gtk::FileChooser::Action::OPEN);
    dialog->set_select_multiple(false);
    dialog->signal_response().connect(sigc::mem_fun(*this, &EditorApplication::on_open_file_dialog_response));
    dialog->show();
}

void EditorApplication::on_open_file_dialog_response(int response) {
    if (response != GTK_RESPONSE_ACCEPT) {
        return;
    }

    auto file    = dialog->get_file();
    auto variant = Glib::Variant<Glib::ustring>::create(file->get_path());
    activate_action("open", variant);
}

void EditorApplication::on_save() {
    auto activeWindow = get_active_window();
    if (activeWindow != nullptr) {
        reinterpret_cast<EditorWindow *>(activeWindow)->save();
    }
}
