#pragma once

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
