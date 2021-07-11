#ifndef GTKMM_EXAMPLEWINDOW_H
#define GTKMM_EXAMPLEWINDOW_H

#include <gtkmm.h>

class ExampleWindow : public Gtk::Window {
  public:
    ExampleWindow();
    virtual ~ExampleWindow();

  protected:
    // Signal handlers:
    void on_button_quit();
    void on_treeview_row_activated(const Gtk::TreeModel::Path &path, Gtk::TreeViewColumn *column);

    // Tree model columns:
    class ModelColumns : public Gtk::TreeModel::ColumnRecord {
      public:
        ModelColumns() {
            add(m_col_id);
            add(m_col_name);
        }

        Gtk::TreeModelColumn<int> m_col_id;
        Gtk::TreeModelColumn<Glib::ustring> m_col_name;
    };

    ModelColumns m_Columns;

    // Child widgets:
    Gtk::Box m_VBox;

    Gtk::ScrolledWindow m_ScrolledWindow;
    Gtk::TreeView m_TreeView;
    Glib::RefPtr<Gtk::TreeStore> m_refTreeModel;

    Gtk::ButtonBox m_ButtonBox;
    Gtk::Button m_Button_Quit;
};

#endif // GTKMM_EXAMPLEWINDOW_H
