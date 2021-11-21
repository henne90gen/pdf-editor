#include "EditorWindow.h"

#include <spdlog/spdlog.h>

EditorWindow::EditorWindow(BaseObjectType *obj, const Glib::RefPtr<Gtk::Builder> &builder, const std::string &filePath)
    : Gtk::ApplicationWindow(obj) {
    if (pdf::Document::read_from_file(filePath, document)) {
        spdlog::error("Failed to open document");
        return;
    }

    Gtk::Builder::get_widget_derived<PdfWindow>(builder, "PdfWindow", document);
}
