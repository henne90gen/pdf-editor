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

#include <pdf_file.h>
#include <pdf_reader.h>

class PdfWidget : public Gtk::DrawingArea {
  public:
    PdfWidget();

    bool on_draw(const Cairo::RefPtr<Cairo::Context> &cr) {
        cr->translate(10, 10);
        cr->show_text("Hello");
        cr->rectangle(0, 0, 200, 200);
        return false;
    }
};

class PdfPage : public Gtk::ScrolledWindow {
  public:
    explicit PdfPage(std::string _fileName);

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
    std::string fileName;
    pdf::File file;
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
