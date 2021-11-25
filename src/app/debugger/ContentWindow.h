#pragma once

// TODO this is a hack to get the gtkmm4 code to compile on Windows
#undef WIN32
#include <gtkmm/adjustment.h>
#include <gtkmm/builder.h>
#include <gtkmm/fixed.h>
#include <gtkmm/scrolledwindow.h>

#include <pdf/document.h>

#include "ContentArea.h"
#include "ScrollableContentWindow.h"

class ContentWindow : public ScrollableContentWindow {
  public:
    [[maybe_unused]] ContentWindow(BaseObjectType *obj, const Glib::RefPtr<Gtk::Builder> &builder,
                                   pdf::Document &document);

    void scroll_to_byte(int byte);

  protected:
    std::pair<double, double> calculate_content_size() override;
    ScrolledContainer &container() override { return *contentArea; }

  private:
    pdf::Document &document;
    ContentArea *contentArea;
};
