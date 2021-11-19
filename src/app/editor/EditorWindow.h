#pragma once

#include <filesystem>
#include <iostream>

// TODO this is a hack to get the gtkmm4 code to compile on Windows
#undef WIN32
#include <gtkmm/applicationwindow.h>
#include <gtkmm/builder.h>

#include "PdfPage.h"

class EditorWindow : public Gtk::ApplicationWindow {
  public:
    [[maybe_unused]] EditorWindow(BaseObjectType *obj, Glib::RefPtr<Gtk::Builder> _builder,
                                  const std::string &filePath);
    ~EditorWindow() override = default;

  protected:
    //    bool custom_on_drag_drop(const Glib::RefPtr<Gdk::DragContext> &context, int x, int y, guint time);
    //
    //    void custom_on_drag_data_received(const Glib::RefPtr<Gdk::DragContext> &context, int x, int y,
    //                                      const Gtk::SelectionData &selection_data, guint info, guint time);

  private:
    pdf::Document document;
    //    PdfPage *pdfPage;
};
