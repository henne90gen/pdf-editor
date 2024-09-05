#include "editor_application.h"

#include <libintl.h>
#include <spdlog/spdlog.h>

std::shared_ptr<EditorApplication> EditorApplication::create() {
    setlocale(LC_ALL, "");
    // TODO: (hmueller) fix translations
    // const auto dirname = working_directory + "/locales";
    // bindtextdomain("timing", dirname.c_str());
    textdomain("timing");

    const auto app = adw_application_new("de.henne90gen.pdf-editor",
                                         (GApplicationFlags)(G_APPLICATION_DEFAULT_FLAGS | G_APPLICATION_HANDLES_OPEN));
    return std::make_shared<EditorApplication>(app);
}

EditorApplication::EditorApplication(AdwApplication *app) : application(app) {
    g_signal_connect(application, "activate", G_CALLBACK(&EditorApplication::on_activate), this);
    g_signal_connect(application, "startup", G_CALLBACK(&EditorApplication::on_startup), this);
    g_signal_connect(application, "open", G_CALLBACK(&EditorApplication::on_open), this);

    const auto open_action = g_simple_action_new("open", G_VARIANT_TYPE_STRING);
    g_signal_connect(open_action, "activate", G_CALLBACK(&EditorApplication::on_open_file), this);
    g_action_map_add_action(G_ACTION_MAP(application), G_ACTION(open_action));

#if 0
    // TODO: (hmueller) add_action("save", G_CALLBACK(&EditorApplication::on_save));
    // TODO: (hmueller) add_action("open_file_dialog", G_CALLBACK(&EditorApplication::on_open_file_dialog));

    // TODO: (hmueller) create file menu
    auto fileMenu = Gio::Menu::create();

    auto openItem = Gio::MenuItem::create("Open", "app.open_file_dialog");
    fileMenu->append_item(openItem);

    auto saveItem = Gio::MenuItem::create("Save", "app.save");
    fileMenu->append_item(saveItem);

    auto menu = Gio::Menu::create();
    menu->append_submenu("File", fileMenu);
    set_menubar(menu);
#endif
}

void EditorApplication::on_activate(GtkApplication *, EditorApplication *) {
    spdlog::info("EditorApplication activated");
    // TODO: (hmueller) open "OpenWindow"
}

void EditorApplication::on_startup(GtkApplication *, EditorApplication *) {
    // TODO: (hmueller) set custom css as soon as it's necessary
    // const auto css_provider = gtk_css_provider_new();
    // gtk_css_provider_load_from_resource(css_provider, "de/henne90gen/pdf/editor/style.css");

    // const auto display = gdk_display_get_default();
    // gtk_style_context_add_provider_for_display(display, GTK_STYLE_PROVIDER(css_provider),
    //                                            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}

void EditorApplication::on_open(GtkApplication *, GFile **files, gint num_files, gchar *, EditorApplication *self) {
    if (num_files == 0) {
        spdlog::warn("There were no files to open");
        return;
    }

    if (num_files == 1) {
        spdlog::info("Opening 1 file");
    } else {
        spdlog::info("Opening {} files", num_files);
    }

    for (int i = 0; i < num_files; i++) {
        const auto filePath = std::string(g_file_get_path(files[i]));
        self->open_window(filePath);
    }
}

void EditorApplication::open_window(const std::string &filePath) {
    spdlog::info("Opening new window for file: {}", filePath);

    if (not windows.contains(filePath)) {
        auto allocator_result = pdf::Allocator::create();
        if (allocator_result.has_error()) {
            spdlog::error("Failed to create allocator: {}", allocator_result.message());
            return;
        }
        allocators.push_back(std::move(allocator_result.value()));

        auto document_result = pdf::Document::read_from_file(allocators.back(), filePath, false);
        if (document_result.has_error()) {
            spdlog::error("Failed to read document from file: {}", document_result.message());
            return;
        }

        const auto builder            = gtk_builder_new_from_resource("/de/henne90gen/pdf-editor/editor.ui");
        const auto application_window = ADW_APPLICATION_WINDOW(gtk_builder_get_object(builder, "EditorWindow"));
        gtk_application_add_window(GTK_APPLICATION(application), GTK_WINDOW(application_window));
        windows[filePath] = std::make_unique<EditorWindow>(builder, application_window, document_result.value());
    }

    auto itr = windows.find(filePath);
    if (itr == windows.end()) {
        spdlog::error("Failed to open window for file {}", filePath);
        return;
    }

    itr->second->present();
}

void EditorApplication::on_open_file(GSimpleAction *, GVariant *parameter, EditorApplication *self) {
    // TODO: (hmueller) find a way to do this check
    // if (g_variant_get_type(parameter) != G_VARIANT_TYPE_STRING) {
    //     spdlog::warn("Parameter for on_open_file is not of type 'string'");
    //     return;
    // }

    gsize length       = 0;
    const auto path    = g_variant_get_string(parameter, &length);
    const auto pathStr = std::string(path, length);
    self->open_window(pathStr);
}

void EditorApplication::on_open_file_dialog(EditorApplication *self) {
    self->dialog = gtk_file_dialog_new();
    // gtk_file_dialog_set_title(self->dialog, _("Open Document"));
    // gtk_file_dialog_open(self->dialog, self->window);
    // TODO: (hmueller) finish migration to GtkFileDialog

    // dialog = Gtk::FileChooserNative::create("Open Document", Gtk::FileChooser::Action::OPEN);
    // dialog->set_select_multiple(false);
    // dialog->signal_response().connect(sigc::mem_fun(*this, &EditorApplication::on_open_file_dialog_response));
    // dialog->show();
}

#if 0
void EditorApplication::on_open_file_dialog_response(EditorApplication *self, int response) {
    if (response != GTK_RESPONSE_ACCEPT) {
        return;
    }

    auto file    = dialog->get_file();
    auto variant = Glib::Variant<Glib::ustring>::create(file->get_path());
    activate_action("open", variant);
}
#endif

void EditorApplication::on_save(EditorApplication *) {
#if 0
    auto activeWindow = get_active_window();
    if (activeWindow != nullptr) {
        reinterpret_cast<EditorWindow *>(activeWindow)->save();
    }
#endif
}
