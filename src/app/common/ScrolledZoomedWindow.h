#pragma once

// TODO this is a hack to get the gtkmm4 code to compile on Windows
#undef WIN32
#include <gtkmm/builder.h>
#include <gtkmm/drawingarea.h>
#include <gtkmm/fixed.h>
#include <gtkmm/scrolledwindow.h>

class ScrolledZoomedContent : public Gtk::DrawingArea {
  public:
    explicit ScrolledZoomedContent(BaseObjectType *obj) : Gtk::DrawingArea(obj) {}

    virtual void set_offsets(double x, double y) = 0;
    virtual void update_zoom(double z)           = 0;
    virtual double zoom()                        = 0;
};

class ScrolledZoomedWindow : public Gtk::ScrolledWindow {
  public:
    [[maybe_unused]] ScrolledZoomedWindow(BaseObjectType *obj, const Glib::RefPtr<Gtk::Builder> &builder,
                                          const std::string &containerName);

  protected:
    void size_allocate_vfunc(int width, int height, int baseline) override;
    bool on_key_pressed(guint keyValue, guint keyCode, Gdk::ModifierType state);
    void on_key_released(guint keyValue, guint keyCode, Gdk::ModifierType state);
    bool on_scroll(double x, double y);
    void update_container_size();

    virtual std::pair<double, double> calculate_content_size() = 0;
    virtual ScrolledZoomedContent &content()                   = 0;

  private:
    Gtk::Fixed *container;
    bool isControlDown = false;

    void scroll_value_changed();
};
