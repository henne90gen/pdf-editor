#include "EditorWindow.h"

#include <gtkmm/droptarget.h>
#include <spdlog/spdlog.h>

#include "PdfWindow.h"

EditorWindow::EditorWindow(BaseObjectType *obj, const Glib::RefPtr<Gtk::Builder> &builder, const std::string &filePath)
    : Gtk::ApplicationWindow(obj) {
    auto result = pdf::Document::read_from_file(filePath);
    if (result.has_error()) {
        spdlog::error(result.message());
        return;
    }
    document = result.value();

    set_title(filePath);
    set_show_menubar(true);

    auto pdfWindow = Gtk::Builder::get_widget_derived<PdfWindow>(builder, "PdfWindow", document);
    pdfWindow->document_changed_signal().connect(sigc::mem_fun(*this, &EditorWindow::on_document_change));

    auto dropTarget = Gtk::DropTarget::create(GDK_TYPE_FILE_LIST, Gdk::DragAction::COPY);
    dropTarget->signal_enter().connect(sigc::mem_fun(*this, &EditorWindow::on_dnd_enter), false);
    dropTarget->signal_motion().connect(sigc::mem_fun(*this, &EditorWindow::on_dnd_motion), false);
    dropTarget->signal_drop().connect(sigc::mem_fun(*this, &EditorWindow::on_dnd_drop), false);
    add_controller(dropTarget);
}

void EditorWindow::save() {
    if (document.file.path.empty()) {
        spdlog::info("No file path available for current document");
        return;
    }

    spdlog::info("Saving {}", document.file.path);
    auto result = document.write_to_file(document.file.path + "_copy.pdf");
    if (result.has_error()) {
        spdlog::warn("Failed to save document: {}", result.message());
    }
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
