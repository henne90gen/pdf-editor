#pragma once

// TODO this is a hack to get the gtkmm4 code to compile on Windows
#undef WIN32
#include <gtkmm/builder.h>
#include <gtkmm/drawingarea.h>
#include <gtkmm/fixed.h>
#include <gtkmm/scrolledwindow.h>

class ScrolledContainer : public Gtk::DrawingArea {
  public:
    explicit ScrolledContainer(BaseObjectType *obj) : Gtk::DrawingArea(obj) {}

    virtual void set_offsets(double x, double y) = 0;
    virtual void update_zoom(double z)           = 0;
    virtual double zoom()                        = 0;
};

class ScrollableContentWindow : public Gtk::ScrolledWindow {
  public:
    [[maybe_unused]] ScrollableContentWindow(BaseObjectType *obj, const Glib::RefPtr<Gtk::Builder> &builder,
                                             const std::string &containerName);

  protected:
    void size_allocate_vfunc(int width, int height, int baseline) override;
    bool on_key_pressed(guint keyValue, guint keyCode, Gdk::ModifierType state);
    void on_key_released(guint keyValue, guint keyCode, Gdk::ModifierType state);
    bool on_scroll(double x, double y);
    void update_container_size();

    virtual std::pair<double, double> calculate_content_size() = 0;
    virtual ScrolledContainer &container()                     = 0;

  private:
    Gtk::Fixed *contentContainer;
    bool isControlDown = false;

    void scroll_value_changed();
};
