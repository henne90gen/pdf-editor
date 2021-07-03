#pragma once

#include "pdf_lexer.h"
#include "pdf_objects.h"
#include "pdf_parser.h"

namespace pdf {

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

struct PageTreeNode;

struct File : public ReferenceResolver {
    char *data                              = nullptr;
    int64_t sizeInBytes                     = 0;
    Trailer trailer                         = {};
    CrossReferenceTable crossReferenceTable = {};
    std::vector<Object *> objects           = {};

    Object *getObject(int64_t objectNumber);
    Dictionary *getRoot();
    PageTreeNode *getPages();
    std::vector<Object *> getAllObjects();
    Object *resolve(IndirectReference *ref) override;

  private:
    [[nodiscard]] Object *loadObject(int64_t objectNumber) const;
};

struct PageTreeNode : public Dictionary {

    Name *type() { return values["Type"]->as<Name>(); }

    PageTreeNode *parent(File &file) {
        auto itr = values.find("Parent");
        if (itr == values.end()) {
            return nullptr;
        }
        auto reference         = itr->second->as<IndirectReference>();
        auto resolvedReference = file.resolve(reference);
        return resolvedReference->as<PageTreeNode>();
    }

    Object *attribute(File &file, const std::string &attributeName, bool inheritable) {
        auto itr = values.find(attributeName);
        if (itr == values.end()) {
            if (inheritable) {
                return parent(file)->attribute(file, attributeName, inheritable);
            } else {
                return nullptr;
            }
        }
        return itr->second->as<IndirectReference>();
    }
};

struct IntermediateNode : public PageTreeNode {
    Array *kids() { return values["Kids"]->as<Array>(); }
    Integer *count() { return values["Count"]->as<Integer>(); }
};

struct PageNode : public PageTreeNode {
    Dictionary *resources(File &file) { return attribute(file, "Resources", true)->as<Dictionary>(); }
};

} // namespace pdf
