#pragma once

// TODO this is a hack to get the gtkmm4 code to compile on Windows
#undef WIN32
#include <gtkmm/adjustment.h>
#include <gtkmm/builder.h>
#include <gtkmm/fixed.h>
#include <gtkmm/scrolledwindow.h>

#include <pdf/document.h>

#include "ContentArea.h"

class ContentWindow : public Gtk::ScrolledWindow {
  public:
    [[maybe_unused]] ContentWindow(BaseObjectType *obj, const Glib::RefPtr<Gtk::Builder> &builder,
                                   pdf::Document &document);

    void scroll_to_byte(int byte);

  protected:
    void size_allocate_vfunc(int width, int height, int baseline) override;

  private:
    Gtk::Fixed *contentContainer;
    ContentArea *contentArea;

    void scroll_value_changed();
};
