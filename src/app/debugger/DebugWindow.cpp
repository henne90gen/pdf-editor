#include "DebugWindow.h"

#include <gtkmm/eventcontrollermotion.h>
#include <spdlog/spdlog.h>

DebugWindow::DebugWindow(BaseObjectType *obj, const Glib::RefPtr<Gtk::Builder> &builder, const std::string &filePath)
    : Gtk::ApplicationWindow(obj) {
    if (pdf::Document::read_from_file(filePath, document)) {
        spdlog::error("Failed to open document");
        return;
    }

    selectedByteLabel = builder->get_widget<Gtk::Label>("SelectedByteLabel");
    hoveredByteLabel  = builder->get_widget<Gtk::Label>("HoveredByteLabel");
    trailerHighlight  = builder->get_widget<Gtk::CheckButton>("TrailerHighlightCheckButton");
    objectsHighlight  = builder->get_widget<Gtk::CheckButton>("ObjectsHighlightCheckButton");
    jumpToByteButton  = builder->get_widget<Gtk::Button>("JumpToByteButton");
    memoryUsageLabel  = builder->get_widget<Gtk::Label>("MemoryUsageLabel");
    memoryUsageLabel->set_text(std::to_string(document.allocator.total_bytes_allocated()));

    contentArea = Gtk::Builder::get_widget_derived<ContentArea>(builder, "ContentArea", document);
    trailerHighlight->signal_toggled().connect(sigc::mem_fun(*contentArea, &ContentArea::toggle_highlight_trailer));
    objectsHighlight->signal_toggled().connect(sigc::mem_fun(*contentArea, &ContentArea::toggle_highlight_objects));
    contentArea->signal_selected_byte().connect(sigc::mem_fun(*this, &DebugWindow::update_selected_byte_label));
    contentArea->signal_hovered_byte().connect(sigc::mem_fun(*this, &DebugWindow::update_hovered_byte_label));

    jumpToByteButton->signal_clicked().connect(sigc::mem_fun(*this, &DebugWindow::open_jump_to_byte_dialog));

    Gtk::Builder::get_widget_derived<ContentWindow>(builder, "ContentWindow", document);
}

void DebugWindow::update_selected_byte_label(int b) {
    if (b == -1) {
        selectedByteLabel->set_text("");
    } else {
        std::string str = std::to_string(b);
        selectedByteLabel->set_text(str);
    }
}

void DebugWindow::update_hovered_byte_label(int b) {
    if (b == -1) {
        hoveredByteLabel->set_text("");
    } else {
        std::string str = std::to_string(b);
        hoveredByteLabel->set_text("(" + str + ")");
    }
}

void DebugWindow::open_jump_to_byte_dialog() {
    jumpToByteDialog = new JumpToByteDialog(*this);
    jumpToByteDialog->signal_response().connect(sigc::mem_fun(*this, &DebugWindow::response_jump_to_byte_dialog));
    jumpToByteDialog->show();
}

void DebugWindow::response_jump_to_byte_dialog(int response) {
    if (jumpToByteDialog == nullptr) {
        spdlog::warn("'jumpToByteDialog' is unexpectedly a nullptr");
        return;
    }

    if (response == Gtk::ResponseType::APPLY) {
        auto byte = jumpToByteDialog->get_byte_value();
        contentArea->set_selected_byte(byte);
    }

    delete jumpToByteDialog;
    jumpToByteDialog = nullptr;
}
