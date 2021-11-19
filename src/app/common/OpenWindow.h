#pragma once

// TODO this is a hack to get the gtkmm4 code to compile on Windows
#undef WIN32
#include <gtkmm/applicationwindow.h>
#include <gtkmm/builder.h>
#include <gtkmm/button.h>
#include <gtkmm/headerbar.h>

class OpenWindow : public Gtk::ApplicationWindow {
  public:
    OpenWindow(BaseObjectType *obj, const Glib::RefPtr<Gtk::Builder> &builder,
               const std::function<void(const std::string &)> &_func);

    void open_document();

  private:
    const std::function<void(const std::string &)> &func;
    Gtk::HeaderBar *titlebar;
    Gtk::Button *openButton;
};
