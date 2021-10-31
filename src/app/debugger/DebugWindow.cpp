#include "DebugWindow.h"

#include <gtkmm/eventcontrollermotion.h>
#include <gtkmm/overlay.h>
#include <spdlog/spdlog.h>

#include "ContentContainer.h"

DebugWindow::DebugWindow(BaseObjectType *obj, const Glib::RefPtr<Gtk::Builder> &builder, pdf::Document _document)
    : Gtk::ApplicationWindow(obj), document(std::move(_document)) {
    byteLabel        = builder->get_widget<Gtk::Label>("ByteLabel");
    contentContainer = Gtk::Builder::get_widget_derived<ContentContainer>(builder, "ContentContainer", document);
    contentContainer->signal_selected_byte().connect(sigc::mem_fun(*this, &DebugWindow::update_byte_label));
}

void DebugWindow::update_byte_label(int b) {
    // calling queue_allocate() before and after setting the text prevents the main view from flickering (this might not
    // be the best solution for that problem)
    byteLabel->queue_allocate();
    if (b == -1) {
        byteLabel->set_text("");
    } else {
        std::string str = std::to_string(b);
        byteLabel->set_text(str);
    }
    byteLabel->queue_allocate();
}
