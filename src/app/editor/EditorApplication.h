#pragma once

// TODO this is a hack to get the gtkmm4 code to compile on Windows
#undef WIN32
#include <gtkmm/application.h>

class EditorApplication : public Gtk::Application {
  public:
    static Glib::RefPtr<EditorApplication> create() {
        return Glib::make_refptr_for_instance<EditorApplication>(new EditorApplication());
    }

  protected:
    EditorApplication();

    void on_activate() override;
    void on_open(const Gio::Application::type_vec_files &files, const Glib::ustring &hint) override;
};
