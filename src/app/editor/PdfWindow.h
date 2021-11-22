#pragma once

// TODO this is a hack to get the gtkmm4 code to compile on Windows
#undef WIN32
#include <gtkmm/builder.h>
#include <gtkmm/fixed.h>
#include <gtkmm/scrolledwindow.h>
#include <pdf/document.h>

#include "PdfArea.h"

class PdfWindow : public Gtk::ScrolledWindow {
  public:
    [[maybe_unused]] PdfWindow(BaseObjectType *obj, const Glib::RefPtr<Gtk::Builder> &builder, pdf::Document &document);

  protected:
    void size_allocate_vfunc(int width, int height, int baseline) override;
    bool on_key_pressed(guint keyValue, guint keyCode, Gdk::ModifierType state);
    void on_key_released(guint keyValue, guint keyCode, Gdk::ModifierType state);

  private:
    Gtk::Fixed *pdfContainer;
    PdfArea *pdfArea;
    double previousHAdjustment = 0.0;
    double previousVAdjustment = 0.0;

    void scroll_value_changed();
};
