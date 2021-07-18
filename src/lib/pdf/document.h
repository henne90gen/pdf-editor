#pragma once

#include "lexer.h"
#include "objects.h"
#include "parser.h"

namespace pdf {

struct PageTreeNode;
struct Page;

struct Trailer {
    int64_t lastCrossRefStart = {};
    Dictionary *dict          = nullptr;
};

struct CrossReferenceEntry {
    int64_t byteOffset       = 0;
    int64_t generationNumber = 0;
    bool isFree              = false;
};

struct CrossReferenceTable {
    int64_t firstObjectId                    = 0;
    int64_t objectCount                      = 0;
    std::vector<CrossReferenceEntry> entries = {};
};

struct Document : public ReferenceResolver {
    char *data                              = nullptr;
    int64_t sizeInBytes                     = 0;
    Trailer trailer                         = {};
    CrossReferenceTable crossReferenceTable = {};
    std::vector<IndirectObject *> objects   = {};

    Dictionary *root();
    std::vector<Page *> pages();
    std::vector<IndirectObject *> getAllObjects();
    IndirectObject *resolve(const IndirectReference *ref) override;

    template <typename T> T *get(Object *object) {
        if (object->is<T>()) {
            return dynamic_cast<T *>(object);
        } else if (object->is<IndirectReference>()) {
            return resolve(object->as<IndirectReference>())->object->as<T>();
        } else if (object->is<IndirectObject>()) {
            return object->as<IndirectObject>()->object->as<T>();
        }
        ASSERT(false);
        return nullptr;
    }

    template <typename T> std::optional<T *> get(std::optional<Object *> object) {
        if (object.has_value()) {
            return get<T>(object.value());
        }
        return {};
    }

    static bool load_from_file(const std::string &filePath, Document &document);

  private:
    IndirectObject *getObject(int64_t objectNumber);
    [[nodiscard]] IndirectObject *loadObject(int64_t objectNumber) const;
};

struct Rectangle : public Array {
    Integer *lowerLeftX() { return values[0]->as<Integer>(); }
    Integer *lowerLeftY() { return values[1]->as<Integer>(); }
    Integer *upperRightX() { return values[2]->as<Integer>(); }
    Integer *upperRightY() { return values[3]->as<Integer>(); }
};

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

        if (inheritable) {
            const std::optional<T *> &attrib = parent(document)->attribute<T>(document, attributeName, inheritable);
            if (!attrib.has_value()) {
                return {};
            }
            return attrib.value()->template as<T>();
        } else {
            return {};
        }
    }
};

struct Page {
    explicit Page(Document &_document, PageTreeNode *_node) : document(_document), node(_node) {}

    Dictionary *resources() { return node->attribute<Dictionary>(document, "Resources", true).value(); }
    Rectangle *mediaBox() { return node->attribute<Rectangle>(document, "MediaBox", true).value(); }

    // TODO make mediaBox the default value for cropBox, bleedBox, trimBox and artBox
    std::optional<Rectangle *> cropBox() { return node->attribute<Rectangle>(document, "CropBox", true); }
    std::optional<Rectangle *> bleedBox() { return node->attribute<Rectangle>(document, "BleedBox", true); }
    std::optional<Rectangle *> trimBox() { return node->attribute<Rectangle>(document, "TrimBox", true); }
    std::optional<Rectangle *> artBox() { return node->attribute<Rectangle>(document, "ArtBox", true); }
    std::optional<Dictionary *> boxColorInfo() { return node->attribute<Dictionary>(document, "BoxColorInfo", false); }
    std::optional<Object *> contents() { return node->attribute<Object>(document, "Contents", false); }
    int64_t rotate();

    Document &document;
    PageTreeNode *node;
};

} // namespace pdf
