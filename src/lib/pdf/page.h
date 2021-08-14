#pragma once

#include "document.h"
#include "objects.h"

namespace pdf {

struct PageTreeNode : public Dictionary {
    Name *type() { return values["Type"]->as<Name>(); }
    bool isPage() { return type()->value == "Page"; }
    PageTreeNode *parent(Document &document);
    Array *kids() { return values["Kids"]->as<Array>(); }
    Integer *count() { return values["Count"]->as<Integer>(); }

    template <typename T>
    std::optional<T *> attribute(Document &document, const std::string &attributeName, bool inheritable) {
        auto itr = values.find(attributeName);
        if (itr != values.end()) {
            return document.get<T>(itr->second);
        }

        if (!inheritable) {
            return {};
        }

        auto p = parent(document);
        if (p == nullptr) {
            return {};
        }

        const std::optional<T *> &attrib = p->attribute<T>(document, attributeName, inheritable);
        if (!attrib.has_value()) {
            return {};
        }

        return attrib.value()->template as<T>();
    }
};

struct Resources : public Dictionary {
    std::optional<FontMap *> fonts(Document &document) { return document.get<FontMap>(find<Object>("Font")); }
};

struct Page {
    explicit Page(Document &_document, PageTreeNode *_node) : document(_document), node(_node) {}

    Resources *resources() { return node->attribute<Resources>(document, "Resources", true).value(); }
    Rectangle *mediaBox() { return node->attribute<Rectangle>(document, "MediaBox", true).value(); }

    // TODO make value_or more efficient (currently mediaBox is fetched even if it is not being used)
    Rectangle *cropBox() { return node->attribute<Rectangle>(document, "CropBox", true).value_or(mediaBox()); }
    Rectangle *bleedBox() { return node->attribute<Rectangle>(document, "BleedBox", true).value_or(mediaBox()); }
    Rectangle *trimBox() { return node->attribute<Rectangle>(document, "TrimBox", true).value_or(mediaBox()); }
    Rectangle *artBox() { return node->attribute<Rectangle>(document, "ArtBox", true).value_or(mediaBox()); }
    std::optional<Dictionary *> boxColorInfo() { return node->attribute<Dictionary>(document, "BoxColorInfo", false); }
    std::optional<Object *> contents() { return node->attribute<Object>(document, "Contents", false); }
    int64_t rotate();
    double width();
    double height();

    Document &document;
    PageTreeNode *node;
};

} // namespace pdf
