#pragma once

#include <gtkmm/adjustment.h>
#include <gtkmm/builder.h>
#include <gtkmm/fixed.h>
#include <gtkmm/scrolledwindow.h>

#include <pdf/document.h>

#include "ContentArea.h"
#include "ScrolledZoomedWindow.h"

class ContentWindow : public ScrolledZoomedWindow {
  public:
    [[maybe_unused]] ContentWindow(BaseObjectType *obj, const Glib::RefPtr<Gtk::Builder> &builder,
                                   pdf::Document &document);

    void scroll_to_byte(int byte);

  protected:
    std::pair<double, double> calculate_content_size() override;
    ScrolledZoomedContent &content() override { return *contentArea; }

  private:
    pdf::Document &document;
    ContentArea *contentArea;
};
