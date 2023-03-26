#pragma once

#include <filesystem>
#include <gtkmm/applicationwindow.h>
#include <gtkmm/builder.h>
#include <iostream>

#include "PdfArea.h"

class EditorWindow : public Gtk::ApplicationWindow {
  public:
    [[maybe_unused]] EditorWindow(BaseObjectType *obj, const Glib::RefPtr<Gtk::Builder> &builder,
                                  const std::string &filePath);

    void save();

  private:
    pdf::Document document;

    void on_document_change();
    Gdk::DragAction on_dnd_enter(double x, double y);
    Gdk::DragAction on_dnd_motion(double x, double y);
    bool on_dnd_drop(const Glib::ValueBase &value, double x, double y);
};
