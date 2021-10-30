#pragma once

#include <gtkmm/applicationwindow.h>
#include <gtkmm/viewport.h>

#include "ContentArea.h"

class ContentContainer : public Gtk::Viewport {
  public:
    [[maybe_unused]] ContentContainer(BaseObjectType *obj, const Glib::RefPtr<Gtk::Builder> &_builder,
                                      Gtk::DrawingArea *_contentArea, pdf::Document &_document);

  protected:
    void size_allocate_vfunc(int w, int h, int baseline) override;

  private:
    pdf::Document &document;
    Gtk::DrawingArea *contentArea;
};

class DebugWindow : public Gtk::ApplicationWindow {
  public:
    [[maybe_unused]] DebugWindow(BaseObjectType *obj, const Glib::RefPtr<Gtk::Builder> &_builder,
                                 pdf::Document _document);

  private:
    pdf::Document document;
    [[maybe_unused]] ContentContainer *contentContainer;
    ContentArea *contentArea;
};
