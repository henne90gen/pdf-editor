#pragma once

// TODO this is a hack to get the gtkmm4 code to compile on Windows
#undef WIN32
#include <gtkmm/application.h>

#include "common/PdfApplication.h"

class EditorApplication : public PdfApplication {
  public:
    static Glib::RefPtr<EditorApplication> create() {
        return Glib::make_refptr_for_instance<EditorApplication>(new EditorApplication());
    }

    void open_window(const std::string &filePath) override;

  protected:
    EditorApplication();
};
