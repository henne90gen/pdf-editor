#pragma once

#include <gtkmm/applicationwindow.h>
#include <gtkmm/drawingarea.h>
#include <gtkmm/label.h>
#include <gtkmm/viewport.h>

#include "ContentContainer.h"

class DebugWindow : public Gtk::ApplicationWindow {
  public:
    [[maybe_unused]] DebugWindow(BaseObjectType *obj, const Glib::RefPtr<Gtk::Builder> &builder,
                                 pdf::Document _document);

    void update_byte_label(int b);

  private:
    pdf::Document document;
    Gtk::Label *byteLabel;
    ContentContainer *contentContainer;
};
