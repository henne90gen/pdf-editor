#pragma once

#include <gtkmm/builder.h>
#include <gtkmm/fixed.h>
#include <gtkmm/scrolledwindow.h>
#include <pdf/document.h>

#include "PdfArea.h"

#include "ScrolledZoomedWindow.h"

class PdfWindow : public ScrolledZoomedWindow {
  public:
    [[maybe_unused]] PdfWindow(BaseObjectType *obj, const Glib::RefPtr<Gtk::Builder> &builder, pdf::Document &document);

    sigc::signal<void()> document_changed_signal() { return pdfArea->documentChangedSignal; }

  protected:
    std::pair<double, double> calculate_content_size() override;
    ScrolledZoomedContent &content() override;

  private:
    pdf::Document &document;
    PdfArea *pdfArea;
};
