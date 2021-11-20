#include "DocumentTree.h"

DocumentTree::DocumentTree(BaseObjectType *obj, const Glib::RefPtr<Gtk::Builder> & /*builder*/,
                           pdf::Document &_document)
    : Gtk::TreeView(obj), document(_document) {
    treeStore = Gtk::TreeStore::create(columns);
    set_model(treeStore);
}

void DocumentTree::fill_tree() {
    spdlog::info("Filling document tree view");
    if (document.trailer.dict != nullptr) {
        auto &row           = *treeStore->append();
        row[columns.name]   = "Trailer";
        auto alreadyVisited = std::unordered_set<pdf::Object *>();
        create_rows(document.trailer.dict, row, alreadyVisited);
    }
}

void DocumentTree::create_row(pdf::Object *object, Gtk::TreeRow &parentRow,
                              std::unordered_set<pdf::Object *> &alreadyVisited) {
    if (alreadyVisited.find(object) != alreadyVisited.end()) {
        return;
    }

    alreadyVisited.insert(object);

    switch (object->type) {
    case pdf::Object::Type::BOOLEAN:
        if (object->as<pdf::Boolean>()->value) {
            parentRow[columns.value] = "true";
        } else {
            parentRow[columns.value] = "false";
        }
        break;
    case pdf::Object::Type::NULL_OBJECT:
        parentRow[columns.value] = "null";
        break;
    case pdf::Object::Type::INTEGER:
        parentRow[columns.value] = std::to_string(object->as<pdf::Integer>()->value);
        break;
    case pdf::Object::Type::REAL:
        parentRow[columns.value] = std::to_string(object->as<pdf::Real>()->value);
        break;
    case pdf::Object::Type::HEXADECIMAL_STRING:
        parentRow[columns.value] = std::string(object->as<pdf::HexadecimalString>()->data);
        break;
    case pdf::Object::Type::LITERAL_STRING:
        parentRow[columns.value] = std::string(object->as<pdf::LiteralString>()->value());
        break;
    case pdf::Object::Type::NAME:
        parentRow[columns.value] = std::string(object->as<pdf::Name>()->value());
        break;
    case pdf::Object::Type::ARRAY:
        create_rows(object->as<pdf::Array>(), parentRow, alreadyVisited);
        break;
    case pdf::Object::Type::DICTIONARY:
        create_rows(object->as<pdf::Dictionary>(), parentRow, alreadyVisited);
        break;
    case pdf::Object::Type::INDIRECT_REFERENCE: {
        auto ref                 = object->as<pdf::IndirectReference>();
        auto indirectObjectLabel = std::to_string(ref->objectNumber) + " " + std::to_string(ref->generationNumber);
        parentRow[columns.value] = indirectObjectLabel;
        auto &row                = parentRow;
        auto newObj              = document.get<pdf::Object>(ref);
        if (newObj->is<pdf::Boolean>() ||           //
            newObj->is<pdf::Null>() ||              //
            newObj->is<pdf::Integer>() ||           //
            newObj->is<pdf::Real>() ||              //
            newObj->is<pdf::HexadecimalString>() || //
            newObj->is<pdf::LiteralString>() ||     //
            newObj->is<pdf::Name>()) {
            row               = *treeStore->append(parentRow.children());
            row[columns.name] = "obj " + indirectObjectLabel;
        }
        create_row(newObj, parentRow, alreadyVisited);
    } break;
    case pdf::Object::Type::STREAM:
        create_row(object->as<pdf::Stream>()->dictionary, parentRow, alreadyVisited);
        break;
    case pdf::Object::Type::OBJECT_STREAM_CONTENT:
        // TODO find out how to display this
        break;
    case pdf::Object::Type::INDIRECT_OBJECT:
    case pdf::Object::Type::OBJECT:
        // ignored
        break;
    }

    alreadyVisited.erase(object);
}

void DocumentTree::create_rows(pdf::Dictionary *dictionary, Gtk::TreeRow &parentRow,
                               std::unordered_set<pdf::Object *> &alreadyVisited) {
    for (auto &entry : dictionary->values) {
        auto &row         = *treeStore->append(parentRow.children());
        row[columns.name] = entry.key;
        create_row(entry.value, row, alreadyVisited);
    }
}

void DocumentTree::create_rows(pdf::Array *array, Gtk::TreeRow &parentRow,
                               std::unordered_set<pdf::Object *> &alreadyVisited) {
    for (size_t i = 0; i < array->values.size(); i++) {
        auto &row         = *treeStore->append(parentRow.children());
        row[columns.name] = std::to_string(i);
        create_row(array->values[i], row, alreadyVisited);
    }
}
