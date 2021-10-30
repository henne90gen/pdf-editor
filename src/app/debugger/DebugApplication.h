#pragma once

#include <gtkmm/application.h>

class DebugApplication : public Gtk::Application {
  public:
    static Glib::RefPtr<DebugApplication> create() {
        return Glib::make_refptr_for_instance<DebugApplication>(new DebugApplication());
    }

  protected:
    DebugApplication();

    void on_activate() override;
    void on_open(const Gio::Application::type_vec_files &files, const Glib::ustring &hint) override;
};
