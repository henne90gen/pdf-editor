#pragma once

#include <filesystem>
#include <iostream>

// TODO this is a hack to get the gtkmm4 code to compile on Windows
#undef WIN32
#include <gtkmm/applicationwindow.h>
#include <gtkmm/builder.h>

#include "PdfStuff.h"

class EditorWindow : public Gtk::ApplicationWindow {
  public:
    [[maybe_unused]] EditorWindow(BaseObjectType *obj, const Glib::RefPtr<Gtk::Builder> &builder,
                                  const std::string &filePath);

  private:
    pdf::Document document;
};
