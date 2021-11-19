#include "EditorWindow.h"

#include <spdlog/spdlog.h>

EditorWindow::EditorWindow(BaseObjectType *obj, Glib::RefPtr<Gtk::Builder> /*_builder*/, const std::string &filePath)
    : Gtk::ApplicationWindow(obj) {
    if (pdf::Document::read_from_file(filePath, document)) {
        spdlog::error("Failed to open document");
        return;
    }

    //    std::vector<Gtk::TargetEntry> targets = {Gtk::TargetEntry("text/uri-list")};
    //    this->drag_dest_set(targets, Gtk::DEST_DEFAULT_DROP | Gtk::DEST_DEFAULT_HIGHLIGHT | Gtk::DEST_DEFAULT_MOTION,
    //                        Gdk::ACTION_COPY | Gdk::ACTION_MOVE);
    //    this->signal_drag_drop().connect(sigc::mem_fun(this, &EditorWindow::custom_on_drag_drop));
    //    this->signal_drag_data_received().connect(sigc::mem_fun(this, &EditorWindow::custom_on_drag_data_received));
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
