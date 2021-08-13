#pragma once

#include <iostream>

#include <glibmm/convert.h>
#include <glibmm/markup.h>
#include <gtkmm/box.h>
#include <gtkmm/drawingarea.h>
#include <gtkmm/label.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/treestore.h>
#include <gtkmm/treeview.h>

#include <pdf/document.h>

class PdfWidget : public Gtk::DrawingArea {
  public:
    explicit PdfWidget(pdf::Document &file);

    bool on_draw(const Cairo::RefPtr<Cairo::Context> &cr) override;
    bool on_scroll_event(GdkEventScroll *event) override;
    bool on_button_press_event(GdkEventButton *button_event) override;
    bool on_key_press_event(GdkEventKey *key_event) override;
    bool on_key_release_event(GdkEventKey *key_event) override;

  private:
    pdf::Document &file;

    bool isCtrlPressed = false;
    double zoom        = 1.0;
    double zoomSpeed   = 0.1;
};

class PdfPage : public Gtk::ScrolledWindow {
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
    PdfWidget pdfWidget;
    Gtk::TreeView treeView;
    ModelColumns columns;
    Glib::RefPtr<Gtk::TreeStore> treeStore;

    void addRows(pdf::Object *obj, int depth, Gtk::TreeModel::Row *parentRow = nullptr);
    void addRows(const pdf::Dictionary *dict, int depth, Gtk::TreeModel::Row *parentRow = nullptr);
    void addRows(const pdf::Array *arr, int depth, Gtk::TreeModel::Row *parentRow = nullptr);

    Gtk::TreeModel::Row createRow(Gtk::TreeRow *parentRow);
};
