#include "DebugWindow.h"

#include <glibmm/main.h>
#include <gtkmm/eventcontrollermotion.h>
#include <iomanip>
#include <sstream>

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

    contentArea = Gtk::Builder::get_widget_derived<ContentArea>(builder, "ContentArea", document);
    trailerHighlight->signal_toggled().connect(sigc::mem_fun(*contentArea, &ContentArea::toggle_highlight_trailer));
    objectsHighlight->signal_toggled().connect(sigc::mem_fun(*contentArea, &ContentArea::toggle_highlight_objects));
    contentArea->signal_selected_byte().connect(sigc::mem_fun(*this, &DebugWindow::update_selected_byte_label));
    contentArea->signal_hovered_byte().connect(sigc::mem_fun(*this, &DebugWindow::update_hovered_byte_label));

    jumpToByteButton->signal_clicked().connect(sigc::mem_fun(*this, &DebugWindow::open_jump_to_byte_dialog));

    Gtk::Builder::get_widget_derived<ContentWindow>(builder, "ContentWindow", document);

    memoryUsageLabel  = builder->get_widget<Gtk::Label>("MemoryUsageLabel");
    update_memory_usage_label();
    // NOTE using polling seems to be the only reasonable solution
    Glib::signal_timeout().connect(
          [this]() {
              update_memory_usage_label();
              return true;
          },
          500);
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

std::string format_bytes(size_t bytes) {
    static std::array<std::string, 6> postfixes = {
          "B", "KB", "MB", "GB", "TB", "PB",
    };

    int i    = 0;
    auto tmp = static_cast<double>(bytes);
    while (tmp >= 1024.0) {
        if (i >= postfixes.size()) {
            break;
        }
        tmp /= 1024.0;
        i++;
    }

    std::stringstream stream;
    stream << std::fixed << std::setprecision(2) << tmp;
    stream << " ";
    stream << postfixes[i];
    return stream.str();
}

void DebugWindow::update_memory_usage_label() {
    auto totalBytesUsed      = format_bytes(document.allocator.total_bytes_used());
    auto totalBytesAllocated = format_bytes(document.allocator.total_bytes_allocated());
    auto numAllocations      = std::to_string(document.allocator.num_allocations());
    auto text = totalBytesUsed + " bytes used / " + totalBytesAllocated + " bytes allocated (" + numAllocations +
                " allocations)";
    memoryUsageLabel->set_text(text);
}
