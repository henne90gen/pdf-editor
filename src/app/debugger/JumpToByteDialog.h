#pragma once

#include <gtkmm/button.h>
#include <gtkmm/dialog.h>
#include <gtkmm/entry.h>

class JumpToByteDialog : public Gtk::Dialog {
  public:
    explicit JumpToByteDialog(Gtk::Window &parent);

    int get_byte_value();

  protected:
    void on_key_released(guint keyValue, guint keyCode, Gdk::ModifierType state);

  private:
    Gtk::Entry *byteText;
    Gtk::Button *jumpButton;
    Gtk::Button *cancelButton;

    void on_text_changed();
};
