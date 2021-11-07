#pragma once

// TODO this is a hack to get the gtkmm4 code to compile on Windows
#undef WIN32
#include <gtkmm/applicationwindow.h>
#include <gtkmm/builder.h>
#include <gtkmm/button.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/drawingarea.h>
#include <gtkmm/label.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/viewport.h>

#include <pdf/document.h>

#include "ContentWindow.h"
#include "JumpToByteDialog.h"

class DebugWindow : public Gtk::ApplicationWindow {
  public:
    [[maybe_unused]] DebugWindow(BaseObjectType *obj, const Glib::RefPtr<Gtk::Builder> &builder,
                                 const pdf::Document &_document);

    void update_selected_byte_label(int b);
    void update_hovered_byte_label(int b);
    void open_jump_to_byte_dialog();
    void response_jump_to_byte_dialog(int response);

  protected:
    static bool on_scroll(double dx, double dy);

  private:
    pdf::Document document;
    ContentWindow *contentWindow;
    ContentArea *contentArea;
    Gtk::Label *selectedByteLabel;
    Gtk::Label *hoveredByteLabel;
    Gtk::CheckButton *trailerHighlight;
    Gtk::CheckButton *objectsHighlight;
    Gtk::Button *jumpToByteButton;
    JumpToByteDialog *jumpToByteDialog = nullptr;
};
