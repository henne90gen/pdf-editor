#pragma once

// TODO this is a hack to get the gtkmm4 code to compile on Windows
#undef WIN32
#include <gtkmm/applicationwindow.h>
#include <gtkmm/drawingarea.h>
#include <gtkmm/label.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/viewport.h>

#include "ContentContainer.h"

class DebugWindow : public Gtk::ApplicationWindow {
  public:
    [[maybe_unused]] DebugWindow(BaseObjectType *obj, const Glib::RefPtr<Gtk::Builder> &builder,
                                 const pdf::Document& _document);

    void update_byte_label(int b);

  private:
    pdf::Document document;
    Gtk::Label *byteLabel;
    Gtk::CheckButton *trailerHighlight;
    Gtk::CheckButton *objectsHighlight;
    ContentContainer *contentContainer;
};
