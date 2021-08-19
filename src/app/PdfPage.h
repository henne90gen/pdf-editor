#pragma once

#include <iostream>
#include <spdlog/spdlog.h>

#include <glibmm/convert.h>
#include <glibmm/markup.h>
#include <gtkmm/box.h>
#include <gtkmm/drawingarea.h>
#include <gtkmm/frame.h>
#include <gtkmm/label.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/treestore.h>
#include <gtkmm/treeview.h>
#include <gtkmm/viewport.h>

#include <pdf/document.h>

class PdfWidget : public Gtk::Viewport {
  public:
    explicit PdfWidget(pdf::Document &file);

    bool my_on_draw(const Cairo::RefPtr<Cairo::Context> &cr);
    bool on_scroll_event(GdkEventScroll *event) override;
    bool on_button_press_event(GdkEventButton *button_event) override;
    bool on_key_press_event(GdkEventKey *key_event) override;
    bool on_key_release_event(GdkEventKey *key_event) override;
    void on_size_allocate(Gtk::Allocation &allocation) override;

    void hadjustment_changed() const { spdlog::info("hadjustment value changed"); }
    void vadjustment_changed() const { spdlog::info("vadjustment value changed"); }
    void update_adjustments(const Gtk::Allocation &allocation);

  private:
    pdf::Document &file;
    Glib::RefPtr<Gtk::Adjustment> hadjustment = Gtk::Adjustment::create(0, 0, 1);
    Glib::RefPtr<Gtk::Adjustment> vadjustment = Gtk::Adjustment::create(0, 0, 1);
    Gtk::DrawingArea drawingArea;

    bool isCtrlPressed = false;
    double zoom        = 1.0;
    double zoomSpeed   = 0.1;
};

class PdfPage : public Gtk::Frame {
  public:
    explicit PdfPage(pdf::Document &file);

    class ModelColumns : public Gtk::TreeStore::ColumnRecord {
      public:
        ModelColumns() {
            add(m_col_name);
            add(m_col_value);
        }

        Gtk::TreeModelColumn<Glib::ustring> m_col_name;
        Gtk::TreeModelColumn<Glib::ustring> m_col_value;
    };

  private:
    pdf::Document &file;

    Gtk::Box box;
    Gtk::ScrolledWindow leftScrolledWindow;
    Gtk::ScrolledWindow rightScrolledWindow;

    PdfWidget pdfWidget;

    Gtk::TreeView treeView;
    ModelColumns columns;
    Glib::RefPtr<Gtk::TreeStore> treeStore;

    void addRows(pdf::Object *obj, int depth, Gtk::TreeModel::Row *parentRow = nullptr);
    void addRows(const pdf::Dictionary *dict, int depth, Gtk::TreeModel::Row *parentRow = nullptr);
    void addRows(const pdf::Array *arr, int depth, Gtk::TreeModel::Row *parentRow = nullptr);

    Gtk::TreeModel::Row createRow(Gtk::TreeRow *parentRow);
};
