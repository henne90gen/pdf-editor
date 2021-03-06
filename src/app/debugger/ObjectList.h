#pragma once

#include <giomm/simpleaction.h>
#include <gtkmm/builder.h>
#include <gtkmm/popovermenu.h>
#include <gtkmm/treestore.h>
#include <gtkmm/treeview.h>
#include <pdf/document.h>
#include <unordered_set>

struct DocumentModelColumns : public Gtk::TreeModel::ColumnRecord {
    Gtk::TreeModelColumn<std::string> name;
    Gtk::TreeModelColumn<std::string> value;
    Gtk::TreeModelColumn<pdf::Object *> object;

    DocumentModelColumns() {
        add(name);
        add(value);
        add(object);
    }
};

class ObjectList : public Gtk::TreeView {
  public:
    ObjectList(BaseObjectType *obj, const Glib::RefPtr<Gtk::Builder> &builder, pdf::Document &document);

    void fill_tree();

    using type_signal_object_selected = sigc::signal<void(pdf::Object *)>;
    type_signal_object_selected signal_object_selected() { return signalObjectSelected; }

  protected:
    void on_row_clicked(const Gtk::TreeModel::Path &, Gtk::TreeViewColumn *);

  private:
    pdf::Document &document;
    Glib::RefPtr<Gtk::TreeStore> treeStore;
    bool documentHasBeenParsed   = false;
    DocumentModelColumns columns = DocumentModelColumns();

    type_signal_object_selected signalObjectSelected;

    void create_child_rows(Gtk::TreeRow &parentRow, pdf::Object *object);

    void create_row(pdf::Object *object, Gtk::TreeRow &parentRow, std::unordered_set<pdf::Object *> &alreadyVisited);
    void create_rows(pdf::Dictionary *dictionary, Gtk::TreeRow &parentRow,
                     std::unordered_set<pdf::Object *> &alreadyVisited);
    void create_rows(pdf::Array *array, Gtk::TreeRow &parentRow, std::unordered_set<pdf::Object *> &alreadyVisited);
};
