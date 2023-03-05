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
    set_show_menubar(true);

    document.documentChangedSignal.connect(sigc::mem_fun(*this, &EditorWindow::on_document_change));

    Gtk::Builder::get_widget_derived<PdfWindow>(builder, "PdfWindow", document);
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
