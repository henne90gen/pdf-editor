#pragma once

#include <gtkmm/builder.h>
#include <gtkmm/viewport.h>
#include <sigc++/sigc++.h>

#include <pdf/document.h>

#include "ByteHighlightOverlay.h"
#include "ContentArea.h"

class ContentContainer : public Gtk::Viewport {
  public:
    [[maybe_unused]] ContentContainer(BaseObjectType *obj, const Glib::RefPtr<Gtk::Builder> &_builder,
                                      pdf::Document &_document);

    using type_signal_selected_byte = sigc::signal<void(int)>;
    type_signal_selected_byte signal_selected_byte() { return signalSelectedByte; }

  protected:
    void size_allocate_vfunc(int w, int h, int baseline) override;
    void on_mouse_leave();
    void on_mouse_enter(double x, double y);
    void on_mouse_motion(double x, double y);
    void updateHighlightedByte(double x, double y);

  private:
    pdf::Document &document;
    ContentArea *contentArea;
    type_signal_selected_byte signalSelectedByte;
    ByteHighlightOverlay byteHighlightOverlay;
};
