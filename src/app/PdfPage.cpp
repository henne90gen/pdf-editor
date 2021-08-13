#include "PdfPage.h"
#include <pdf/renderer.h>

PdfPage::PdfPage(pdf::Document &_file) : file(_file), pdfWidget(_file) {
    box.set_orientation(Gtk::ORIENTATION_HORIZONTAL);
    add(box);

    box.pack_start(treeView, false, true);
    box.pack_start(pdfWidget);
    set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);

    treeStore = Gtk::TreeStore::create(columns);
    treeView.set_model(treeStore);

    auto root = file.root();
    ASSERT(root != nullptr);
    addRows(root, 0);

    treeView.append_column("Name", columns.m_col_name);
    treeView.append_column("Value", columns.m_col_value);

    show_all_children();
}

void PdfPage::addRows(pdf::Object *obj, int depth, Gtk::TreeModel::Row *parentRow) {
    // TODO use a hash set to stop infinite recursion
    if (depth > 5) {
        return;
    }
    if (obj == nullptr) {
        // TODO maybe show a special row?
        return;
    }

    switch (obj->type) {
    case pdf::Object::Type::BOOLEAN: {
        ASSERT(parentRow != nullptr);
        if (obj->as<pdf::Boolean>()->value) {
            (*parentRow)[columns.m_col_value] = "true";
        } else {
            (*parentRow)[columns.m_col_value] = "false";
        }
        break;
    }
    case pdf::Object::Type::INTEGER: {
        ASSERT(parentRow != nullptr);
        (*parentRow)[columns.m_col_value] = std::to_string(obj->as<pdf::Integer>()->value);
        break;
    }
    case pdf::Object::Type::REAL: {
        ASSERT(parentRow != nullptr);
        (*parentRow)[columns.m_col_value] = std::to_string(obj->as<pdf::Real>()->value);
        break;
    }
    case pdf::Object::Type::HEXADECIMAL_STRING: {
        ASSERT(parentRow != nullptr);
        auto *hexString                   = obj->as<pdf::HexadecimalString>();
        (*parentRow)[columns.m_col_value] = std::string(hexString->value) + " (" + hexString->to_string() + ")";
        break;
    }
    case pdf::Object::Type::LITERAL_STRING: {
        ASSERT(parentRow != nullptr);
        (*parentRow)[columns.m_col_value] = std::string(obj->as<pdf::LiteralString>()->value);
        break;
    }
    case pdf::Object::Type::NAME: {
        ASSERT(parentRow != nullptr);
        (*parentRow)[columns.m_col_value] = std::string(obj->as<pdf::Name>()->value);
        break;
    }
    case pdf::Object::Type::ARRAY:
        addRows(obj->as<pdf::Array>(), depth, parentRow);
        break;
    case pdf::Object::Type::DICTIONARY:
        addRows(obj->as<pdf::Dictionary>(), depth, parentRow);
        break;
    case pdf::Object::Type::INDIRECT_REFERENCE:
        addRows(file.resolve(obj->as<pdf::IndirectReference>()), depth, parentRow);
        break;
    case pdf::Object::Type::INDIRECT_OBJECT:
        addRows(obj->as<pdf::IndirectObject>()->object, depth, parentRow);
        break;
    case pdf::Object::Type::STREAM:
        ASSERT(parentRow != nullptr);
        // TODO this does not yet work as expected (crashes with "munmap_chunk(): invalid pointer")
        //        (*parentRow)[columns.m_col_value] = std::string(obj->as<pdf::Stream>()->to_string());
        break;
    case pdf::Object::Type::NULL_OBJECT: {
        ASSERT(parentRow != nullptr);
        (*parentRow)[columns.m_col_value] = "null";
        break;
    }
    default:
        ASSERT(false);
    }
}

void PdfPage::addRows(const pdf::Dictionary *dict, int depth, Gtk::TreeModel::Row *parentRow) {
    for (auto &entry : dict->values) {
        auto row                = createRow(parentRow);
        row[columns.m_col_name] = entry.first;
        addRows(entry.second, depth + 1, &row);
    }
}

void PdfPage::addRows(const pdf::Array *arr, int depth, Gtk::TreeModel::Row *parentRow) {
    for (int i = 0; i < arr->values.size(); i++) {
        auto row                = createRow(parentRow);
        row[columns.m_col_name] = std::to_string(i);
        addRows(arr->values[i], depth + 1, &row);
    }
}

Gtk::TreeModel::Row PdfPage::createRow(Gtk::TreeRow *parentRow) {
    if (parentRow == nullptr) {
        return *treeStore->append();
    } else {
        return *treeStore->append(parentRow->children());
    }
}

PdfWidget::PdfWidget(pdf::Document &_file) : file(_file) { add_events(Gdk::SMOOTH_SCROLL_MASK); }

bool PdfWidget::on_draw(const Cairo::RefPtr<Cairo::Context> &cr) {
    cr->scale(zoom, zoom);

    auto pages = file.pages();
    for (auto page : pages) {
        pdf::renderer renderer(page);
        renderer.render(cr);
    }

    return false;
}

bool PdfWidget::on_scroll_event(GdkEventScroll *event) {
    zoom += event->delta_y * zoomSpeed;
    queue_draw();
    return false;
}

bool PdfWidget::on_button_press_event(GdkEventButton *button_event) {
    std::cout << "Button pressed" << std::endl;
    return false;
}
