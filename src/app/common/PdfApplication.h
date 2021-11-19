#pragma once

// TODO this is a hack to get the gtkmm4 code to compile on Windows
#undef WIN32
#include <gtkmm/application.h>

class PdfApplication : public Gtk::Application {
  protected:
    explicit PdfApplication(const std::string &appId);

    virtual void open_window(const std::string &filePath) = 0;

    void on_activate() override;
    void on_open(const Gio::Application::type_vec_files &files, const Glib::ustring &hint) override;
};
