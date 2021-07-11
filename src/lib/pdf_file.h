#pragma once

#include "pdf_lexer.h"
#include "pdf_objects.h"
#include "pdf_parser.h"

namespace pdf {

struct PageTreeNode;
struct PageNode;

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

struct File : public ReferenceResolver {
    char *data                              = nullptr;
    int64_t sizeInBytes                     = 0;
    Trailer trailer                         = {};
    CrossReferenceTable crossReferenceTable = {};
    std::vector<IndirectObject *> objects   = {};

    IndirectObject *getObject(int64_t objectNumber);
    Dictionary *getRoot();
    PageTreeNode *getPageTree();
    std::vector<IndirectObject *> getAllObjects();
    IndirectObject *resolve(const IndirectReference *ref) override;

  private:
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
    PageTreeNode *parent(File &file);
    std::vector<PageNode *> pages(File &file);

    template <typename T> std::optional<T *> attribute(File &file, const std::string &attributeName, bool inheritable) {
        auto itr = values.find(attributeName);
        if (itr == values.end()) {
            if (inheritable) {
                const std::optional<T *> &attrib = parent(file)->attribute<T>(file, attributeName, inheritable);
                if (!attrib.has_value()) {
                    return {};
                }
                return attrib.value()->template as<T>();
            } else {
                return {};
            }
        }
        return itr->second->as<T>();
    }
};

struct IntermediateNode : public PageTreeNode {
    Array *kids() { return values["Kids"]->as<Array>(); }
    Integer *count() { return values["Count"]->as<Integer>(); }
};

struct PageNode : public PageTreeNode {
    Dictionary *resources(File &file) { return attribute<Dictionary>(file, "Resources", true).value(); }
    Rectangle *mediaBox(File &file) { return attribute<Rectangle>(file, "MediaBox", true).value(); }

    // TODO make mediaBox the default value for cropBox, bleedBox, trimBox and artBox
    std::optional<Rectangle *> cropBox(File &file) { return attribute<Rectangle>(file, "CropBox", true); }
    std::optional<Rectangle *> bleedBox(File &file) { return attribute<Rectangle>(file, "BleedBox", true); }
    std::optional<Rectangle *> trimBox(File &file) { return attribute<Rectangle>(file, "TrimBox", true); }
    std::optional<Rectangle *> artBox(File &file) { return attribute<Rectangle>(file, "ArtBox", true); }
    std::optional<Dictionary *> boxColorInfo(File &file) { return attribute<Dictionary>(file, "BoxColorInfo", false); }
    std::optional<Object *> contents(File &file) { return attribute<Object>(file, "Contents", false); }
    int64_t rotate(File &file);
};

} // namespace pdf
