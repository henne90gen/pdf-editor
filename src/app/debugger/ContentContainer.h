#pragma once

#include <gtkmm/viewport.h>
#include <gtkmm/builder.h>

#include <pdf/document.h>

#include "ContentArea.h"
#include "ByteHighlightOverlay.h"

class ContentContainer : public Gtk::Viewport {
  public:
    [[maybe_unused]] ContentContainer(BaseObjectType *obj, const Glib::RefPtr<Gtk::Builder> &_builder,
                                      pdf::Document &_document);

  protected:
    void size_allocate_vfunc(int w, int h, int baseline) override;
    void on_mouse_leave();
    void on_mouse_enter(double x, double y);
    void on_mouse_motion(double x, double y);
    void updateHighlightedByte(double x, double y);

  private:
    pdf::Document &document;
    ContentArea *contentArea;
    ByteHighlightOverlay byteHighlightOverlay;
};
