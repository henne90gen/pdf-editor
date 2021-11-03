#include "EditorWindow.h"

EditorWindow::EditorWindow(BaseObjectType *obj, Glib::RefPtr<Gtk::Builder> _builder)
    : Gtk::ApplicationWindow(obj), builder(std::move(_builder)) {

//    std::vector<Gtk::TargetEntry> targets = {Gtk::TargetEntry("text/uri-list")};
//    this->drag_dest_set(targets, Gtk::DEST_DEFAULT_DROP | Gtk::DEST_DEFAULT_HIGHLIGHT | Gtk::DEST_DEFAULT_MOTION,
//                        Gdk::ACTION_COPY | Gdk::ACTION_MOVE);
//    this->signal_drag_drop().connect(sigc::mem_fun(this, &EditorWindow::custom_on_drag_drop));
//    this->signal_drag_data_received().connect(sigc::mem_fun(this, &EditorWindow::custom_on_drag_data_received));

    notebook = builder->get_widget<Gtk::Notebook>("ContentNotebook");
    for (int i = 0; i < notebook->get_n_pages(); i++) {
        notebook->remove_page(i);
    }

    notebook->show();
}

// bool EditorWindow::custom_on_drag_drop(const Glib::RefPtr<Gdk::DragContext> &context, int x, int y, guint time) {
//     // limits the data that is received to "text/uri-list", prevents on_drag_data_received from being triggered more
//     // than once
//     drag_get_data(context, "text/uri-list", time);
//     return true;
// }

// void EditorWindow::custom_on_drag_data_received(const Glib::RefPtr<Gdk::DragContext> &context, int x, int y,
//                                               const Gtk::SelectionData &selection_data, guint info, guint time) {
//     auto uriList = selection_data.get_uris();
//     for (auto &uri : uriList) {
//         auto fileName = Glib::filename_from_uri(uri);
//         add_pdf_page(fileName);
//     }
//     notebook->show();
//     context->drag_finish(true, false, time);
// }

void EditorWindow::add_document(const std::string &filePath) {
    auto &document = documents.emplace_back();
    if (pdf::Document::read_from_file(filePath, document)) {
        spdlog::error("Could not load pdf file");
        return;
    }

    auto &page     = pages.emplace_back(document);
    auto lastSlash = filePath.find_last_of('/');
    auto fileName  = filePath.substr(lastSlash + 1);
    notebook->append_page(page, fileName);
}