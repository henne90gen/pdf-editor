#pragma once

#include <gtkmm/application.h>

#include <PdfApplication.h>

class EditorApplication : public PdfApplication {
  public:
    static Glib::RefPtr<EditorApplication> create() {
        return Glib::make_refptr_for_instance<EditorApplication>(new EditorApplication());
    }

    void open_window(const std::string &filePath) override;

  protected:
    EditorApplication();
};
