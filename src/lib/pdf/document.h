#pragma once

#include "font.h"
#include "objects.h"
#include "parser.h"

namespace pdf {

struct PageTreeNode;
struct Page;

struct Trailer {
    Dictionary *root(Document &document) const;
    Dictionary *get_dict() { return dict; }
    void set_dict(Dictionary *_dict) { this->dict = _dict; }
    Stream *get_stream() { return stream; }
    void set_stream(Stream *_stream) { this->stream = _stream; }

    int64_t lastCrossRefStart = {};

  private:
    Dictionary *dict = nullptr;
    Stream *stream   = nullptr;
};

enum class CrossReferenceEntryType {
    FREE=0,
    NORMAL=1,
    COMPRESSED=2,
};

struct CrossReferenceEntry {
    CrossReferenceEntryType type;
    union {
        struct {
            uint64_t nextFreeObjectNumber;
            uint64_t nextFreeObjectGenerationNumber;
        } free;
        struct {
            uint64_t byteOffset;
            uint64_t generationNumber;
        } normal;
        struct {
            uint64_t objectNumberOfStream;
            uint64_t indexInStream;
        } compressed;
    };
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
    IndirectObject *getObject(uint64_t objectNumber);
    [[nodiscard]] IndirectObject *loadObject(int64_t objectNumber);
};

} // namespace pdf
