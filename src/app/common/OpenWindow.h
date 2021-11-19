#pragma once

// TODO this is a hack to get the gtkmm4 code to compile on Windows
#undef WIN32
#include <gtkmm/applicationwindow.h>
#include <gtkmm/builder.h>
#include <gtkmm/button.h>
#include <gtkmm/filechooserdialog.h>
#include <gtkmm/headerbar.h>

class OpenWindow : public Gtk::ApplicationWindow {
  public:
    OpenWindow(BaseObjectType *obj, const Glib::RefPtr<Gtk::Builder> &builder,
               std::function<void(const std::string &)> _func);

    void on_open_file();
    void on_open_response(int response);

  private:
    const std::function<void(const std::string &)> func;
    Gtk::HeaderBar *titlebar           = nullptr;
    Gtk::Button *openButton            = nullptr;
    Gtk::FileChooserDialog *openDialog = nullptr;
};
