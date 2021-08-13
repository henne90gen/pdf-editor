#pragma once

#include "font.h"
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
        if (object->is<IndirectReference>()) {
            return resolve(object->as<IndirectReference>())->object->as<T>();
        } else if (object->is<IndirectObject>()) {
            return object->as<IndirectObject>()->object->as<T>();
        } else if (object->is<T>()) {
            return object->as<T>();
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

    [[nodiscard]] bool saveToFile(const std::string &filePath) const;
    static bool loadFromFile(const std::string &filePath, Document &document);

  private:
    IndirectObject *getObject(int64_t objectNumber);
    [[nodiscard]] IndirectObject *loadObject(int64_t objectNumber) const;
};

} // namespace pdf
