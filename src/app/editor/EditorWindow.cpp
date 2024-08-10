#include "EditorWindow.h"

#include <adwaita.h>
#include <pdf/document.h>
#include <spdlog/spdlog.h>

EditorWindow::EditorWindow(EditorApplication &, GtkBuilder *, AdwApplicationWindow *window_, pdf::Document &document_)
    : // app(app_), builder(builder_),
      window(window_), document(std::move(document_)) {
    gtk_window_set_title(GTK_WINDOW(window), document.file.path.c_str());
    gtk_application_window_set_show_menubar(GTK_APPLICATION_WINDOW(window), true);

    // TODO auto pdfWindow = Gtk::Builder::get_widget_derived<PdfWindow>(builder, "PdfWindow", document);
    // TODO pdfWindow->document_changed_signal().connect(sigc::mem_fun(*this, &EditorWindow::on_document_change));

    // TODO auto dropTarget = Gtk::DropTarget::create(GDK_TYPE_FILE_LIST, Gdk::DragAction::COPY);
    // TODO dropTarget->signal_enter().connect(sigc::mem_fun(*this, &EditorWindow::on_dnd_enter), false);
    // TODO dropTarget->signal_motion().connect(sigc::mem_fun(*this, &EditorWindow::on_dnd_motion), false);
    // TODO dropTarget->signal_drop().connect(sigc::mem_fun(*this, &EditorWindow::on_dnd_drop), false);
    // TODO add_controller(dropTarget);
}

#if 0
void EditorWindow::save() {
    auto path = document.file.path;
    if (path.empty()) {
        spdlog::info("No file path available for current document");
        return;
    }

    auto extension = path.substr(path.size() - 4);
    if (extension == ".pdf") {
        path = path.substr(0, path.size() - 4);
    }
    path = path + "_copy.pdf";

    spdlog::info("Saving {}", path);
    auto result = document.write_to_file(path);
    if (result.has_error()) {
        spdlog::warn("Failed to save document: {}", result.message());
        return;
    }

    set_title(path);
}

void EditorWindow::on_document_change() { set_title("* " + document.file.path); }

Gdk::DragAction EditorWindow::on_dnd_enter(double x, double y) {
    spdlog::info("Enter: {} | {}", x, y);
    return Gdk::DragAction::COPY;
}

Gdk::DragAction EditorWindow::on_dnd_motion(double x, double y) {
    spdlog::info("Drag: {} | {}", x, y);
    return Gdk::DragAction::COPY;
}

bool EditorWindow::on_dnd_drop(const Glib::ValueBase &value, double, double) {
    auto obj = value.gobj();
    if (obj->g_type != GDK_TYPE_FILE_LIST) {
        return false;
    }

    auto *fileList = reinterpret_cast<GdkFileList *>(g_value_get_boxed(obj));
    GSList *list   = gdk_file_list_get_files(fileList);

    for (GSList *l = list; l != nullptr; l = l->next) {
        auto *file    = reinterpret_cast<GFile *>(l->data);
        auto filePath = g_file_get_path(file);
        auto variant  = Glib::Variant<Glib::ustring>::create(filePath);
        activate_action("app.open", variant);
    }

    return false;
}

#endif
