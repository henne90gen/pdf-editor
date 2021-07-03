#pragma once

#include <iostream>

#include <glibmm/convert.h>
#include <glibmm/markup.h>
#include <gtkmm/applicationwindow.h>
#include <gtkmm/box.h>
#include <gtkmm/builder.h>
#include <gtkmm/notebook.h>
#include <gtkmm/textview.h>

#include <pdf_file.h>
#include <pdf_reader.h>

class PdfPage : public Gtk::Box {
  public:
    PdfPage(BaseObjectType *obj, Glib::RefPtr<Gtk::Builder> _builder, std::string _fileName);

  private:
    Glib::RefPtr<Gtk::Builder> builder;
    std::string fileName;
    pdf::File file;
};
