#pragma once

#include <gtkmm/application.h>
#include <gtkmm/filechoosernative.h>

#include <PdfApplication.h>

class EditorApplication : public PdfApplication {
  public:
    static Glib::RefPtr<EditorApplication> create() {
        return Glib::make_refptr_for_instance<EditorApplication>(new EditorApplication());
    }

    void open_window(const std::string &filePath) override;

  protected:
    EditorApplication();

  private:
    Glib::RefPtr<Gtk::FileChooserNative> dialog;

    void on_open_file_dialog();
    void on_open_file_dialog_response(int response);
    void on_save();
    void on_open_file(const Glib::VariantBase &);
};
