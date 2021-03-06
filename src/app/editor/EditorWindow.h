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

  private:
    pdf::Document document;
};
