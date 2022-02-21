#include "DebugWindow.h"

#include <glibmm/main.h>
#include <gtkmm/cssprovider.h>
#include <gtkmm/eventcontrollermotion.h>
#include <gtkmm/paned.h>
#include <iomanip>
#include <sstream>

DebugWindow::DebugWindow(BaseObjectType *obj, const Glib::RefPtr<Gtk::Builder> &builder, const std::string &filePath)
    : Gtk::ApplicationWindow(obj) {
    auto result = pdf::Document::read_from_file(filePath, document, false);
    if (result.has_error()) {
        spdlog::error(result.message());
        return;
    }
    set_title(filePath);

    // apply css
    Glib::RefPtr<Gtk::CssProvider> cssProvider = Gtk::CssProvider::create();
    cssProvider->load_from_resource("/com/github/henne90gen/pdf-debugger/style.css");
    get_style_context()->add_provider_for_display(get_display(), cssProvider, GTK_STYLE_PROVIDER_PRIORITY_USER);

    // get widget references
    selectedByteLabel   = builder->get_widget<Gtk::Label>("SelectedByteLabel");
    hoveredByteLabel    = builder->get_widget<Gtk::Label>("HoveredByteLabel");
    memoryUsageLabel    = builder->get_widget<Gtk::Label>("MemoryUsageLabel");
    trailerHighlight    = builder->get_widget<Gtk::CheckButton>("TrailerHighlightCheckButton");
    objectsHighlight    = builder->get_widget<Gtk::CheckButton>("ObjectsHighlightCheckButton");
    jumpToByteButton    = builder->get_widget<Gtk::Button>("JumpToByteButton");
    parseDocumentButton = builder->get_widget<Gtk::Button>("ParseDocumentButton");
    contentArea         = Gtk::Builder::get_widget_derived<ContentArea>(builder, "ContentArea", document);
    documentTree        = Gtk::Builder::get_widget_derived<DocumentTree>(builder, "DocumentTree", document);
    detailsLabel        = builder->get_widget<Gtk::Label>("DetailsLabel");

    // connect signals
    parseDocumentButton->signal_clicked().connect(sigc::mem_fun(*documentTree, &DocumentTree::fill_tree));
    trailerHighlight->signal_toggled().connect(sigc::mem_fun(*contentArea, &ContentArea::toggle_highlight_trailer));
    objectsHighlight->signal_toggled().connect(sigc::mem_fun(*contentArea, &ContentArea::toggle_highlight_objects));
    contentArea->signal_selected_byte().connect(sigc::mem_fun(*this, &DebugWindow::update_selected_byte_label));
    contentArea->signal_hovered_byte().connect(sigc::mem_fun(*this, &DebugWindow::update_hovered_byte_label));
    jumpToByteButton->signal_clicked().connect(sigc::mem_fun(*this, &DebugWindow::open_jump_to_byte_dialog));
    documentTree->signal_object_selected().connect(sigc::mem_fun(*this, &DebugWindow::update_details_label));

    auto gioFile = Gio::File::create_for_path(filePath);
    fileMonitor  = gioFile->monitor_file(Gio::FileMonitorFlags::NONE);
    fileMonitor->signal_changed().connect(sigc::mem_fun(*this, &DebugWindow::document_changed));

    update_memory_usage_label();
    // NOTE using polling seems to be the only reasonable solution
    Glib::signal_timeout().connect(
          [this]() {
              update_memory_usage_label();
              return true;
          },
          500);

    auto mainContentPaned = builder->get_widget<Gtk::Paned>("MainContentPaned");
    mainContentPaned->set_shrink_end_child(false);
    mainContentPaned->set_shrink_start_child(false);
    mainContentPaned->set_resize_end_child(false);
    mainContentPaned->set_resize_start_child(true);

    auto contentWindow = Gtk::Builder::get_widget_derived<ContentWindow>(builder, "ContentWindow", document);
    contentWindow->set_size_request(300, -1);

    auto detailsPane = builder->get_widget<Gtk::Box>("DetailsPane");
    detailsPane->set_size_request(200, -1);
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

    size_t i = 0;
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

void DebugWindow::update_details_label(pdf::Object *object) {
    detailsLabel->set_text("");

    switch (object->type) {
    case pdf::Object::Type::OBJECT:
        break;
    case pdf::Object::Type::BOOLEAN:
        break;
    case pdf::Object::Type::INTEGER:
        break;
    case pdf::Object::Type::REAL:
        break;
    case pdf::Object::Type::HEXADECIMAL_STRING:
        break;
    case pdf::Object::Type::LITERAL_STRING:
        break;
    case pdf::Object::Type::NAME:
        break;
    case pdf::Object::Type::ARRAY:
        break;
    case pdf::Object::Type::DICTIONARY:
        break;
    case pdf::Object::Type::INDIRECT_REFERENCE:
        update_details_label(document.resolve(object->as<pdf::IndirectReference>()));
        break;
    case pdf::Object::Type::INDIRECT_OBJECT:
        update_details_label(object->as<pdf::IndirectObject>()->object);
        break;
    case pdf::Object::Type::STREAM: {
        auto stream = object->as<pdf::Stream>();
        auto str    = std::string(stream->decode(document.allocator));
        detailsLabel->set_text(str);
    } break;
    case pdf::Object::Type::NULL_OBJECT:
        break;
    case pdf::Object::Type::OBJECT_STREAM_CONTENT:
        break;
    }
}

void DebugWindow::document_changed(const Glib::RefPtr<Gio::File> &file, const Glib::RefPtr<Gio::File> &otherFile,
                                   Gio::FileMonitor::Event event) {
    spdlog::trace("DebugWindow::document_changed(file={}, otherFile={}, event={})", file->get_path(),
                  otherFile->get_path(), event);
    if (event != Gio::FileMonitor::Event::CHANGES_DONE_HINT) {
        return;
    }
    // TODO reload document and refresh UI
}
