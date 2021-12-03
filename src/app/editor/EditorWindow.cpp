#include "EditorWindow.h"

#include <spdlog/spdlog.h>

#include "PdfWindow.h"

EditorWindow::EditorWindow(BaseObjectType *obj, const Glib::RefPtr<Gtk::Builder> &builder, const std::string &filePath)
    : Gtk::ApplicationWindow(obj) {
    const auto result = pdf::Document::read_from_file(filePath, document);
    if (result.has_error()) {
        spdlog::error(result.message());
        return;
    }
    set_title(filePath);

    Gtk::Builder::get_widget_derived<PdfWindow>(builder, "PdfWindow", document);
}
