#pragma once

// TODO this is a hack to get the gtkmm4 code to compile on Windows
#undef WIN32
#include <gtkmm/application.h>
#include <gtkmm/window.h>

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
