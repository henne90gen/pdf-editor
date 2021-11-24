#pragma once

// TODO this is a hack to get the gtkmm4 code to compile on Windows
#undef WIN32
#include <gtkmm/builder.h>
#include <gtkmm/fixed.h>
#include <gtkmm/scrolledwindow.h>
#include <pdf/document.h>

#include "PdfArea.h"

#include "common/ScrollableContentWindow.h"

class PdfWindow : public ScrollableContentWindow {
  public:
    [[maybe_unused]] PdfWindow(BaseObjectType *obj, const Glib::RefPtr<Gtk::Builder> &builder, pdf::Document &document);

  protected:
    std::pair<double, double> calculate_content_size() override;
    ScrolledContainer &container() override;

  private:
    pdf::Document &document;
    PdfArea *pdfArea;
};
