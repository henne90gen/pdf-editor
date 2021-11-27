#pragma once

// TODO this is a hack to get the gtkmm4 code to compile on Windows
#undef WIN32
#include <gtkmm/builder.h>
#include <gtkmm/popovermenu.h>
#include <gtkmm/treestore.h>
#include <gtkmm/treeview.h>
#include <pdf/document.h>
#include <unordered_set>

struct DocumentModelColumns : public Gtk::TreeModel::ColumnRecord {
    Gtk::TreeModelColumn<std::string> name;
    Gtk::TreeModelColumn<std::string> value;

    DocumentModelColumns() {
        add(name);
        add(value);
    }
};

class DocumentTree : public Gtk::TreeView {
  public:
    DocumentTree(BaseObjectType *obj, const Glib::RefPtr<Gtk::Builder> &builder, pdf::Document &document);

    void fill_tree();

  protected:
    // Signal handler for showing popup menu:
    void on_popup_button_pressed(int n_press, double x, double y);

    // Signal handler for popup menu items:
    void on_menu_file_popup_generic();

  private:
    pdf::Document &document;
    Glib::RefPtr<Gtk::TreeStore> treeStore;
    DocumentModelColumns columns = DocumentModelColumns();
    Gtk::PopoverMenu menuPopover;

    void create_row(pdf::Object *object, Gtk::TreeRow &parentRow, std::unordered_set<pdf::Object *> &alreadyVisited);
    void create_rows(pdf::Dictionary *dictionary, Gtk::TreeRow &parentRow,
                     std::unordered_set<pdf::Object *> &alreadyVisited);
    void create_rows(pdf::Array *array, Gtk::TreeRow &parentRow, std::unordered_set<pdf::Object *> &alreadyVisited);
};
