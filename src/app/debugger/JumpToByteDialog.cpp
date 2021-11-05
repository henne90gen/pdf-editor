#include "JumpToByteDialog.h"

#include <gdk/gdkkeysyms.h>
#include <gtkmm/eventcontrollerkey.h>

JumpToByteDialog::JumpToByteDialog(Gtk::Window &parent) : Gtk::Dialog("Jump to Byte", parent, true, true) {
    auto contentArea = get_content_area();
    byteText         = new Gtk::Entry();
    byteText->set_text("0");
    byteText->set_input_purpose(Gtk::InputPurpose::DIGITS);
    byteText->signal_changed().connect(sigc::mem_fun(*this, &JumpToByteDialog::on_text_changed));
    auto keyCtrl = Gtk::EventControllerKey::create();
    keyCtrl->signal_key_released().connect(sigc::mem_fun(*this, &JumpToByteDialog::on_key_released));
    byteText->add_controller(keyCtrl);
    contentArea->append(*byteText);

    jumpButton = new Gtk::Button("Jump");
    jumpButton->set_sensitive(false);
    add_action_widget(*jumpButton, Gtk::ResponseType::APPLY);

    cancelButton = new Gtk::Button("Cancel");
    add_action_widget(*cancelButton, Gtk::ResponseType::CANCEL);
}

void JumpToByteDialog::on_text_changed() {
    auto text    = byteText->get_text();
    bool isValid = !text.empty();
    for (const auto &c : text) {
        if (c >= '0' && c <= '9') {
            // valid
        } else {
            isValid = false;
            break;
        }
    }
    jumpButton->set_sensitive(isValid);
}

int JumpToByteDialog::get_byte_value() {
    const Glib::ustring &text = byteText->get_text();
    return std::stoi(text);
}

void JumpToByteDialog::on_key_released(guint keyValue, guint /*keyCode*/, Gdk::ModifierType /*state*/) {
    if (keyValue != GDK_KEY_Return) {
        return;
    }
    if (!jumpButton->is_sensitive()) {
        return;
    }

    response(Gtk::ResponseType::APPLY);
}
