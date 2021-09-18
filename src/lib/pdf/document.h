#pragma once

#include "font.h"
#include "objects.h"
#include "parser.h"

#include <functional>

namespace pdf {

struct PageTreeNode;
struct Page;
struct Document;

struct DocumentCatalog : Dictionary {
    PageTreeNode *page_tree_root(Document &document);
    /*
        Object *outline_hierarchy();
        Object *article_threads();
        Object *named_destinations();
        Object *interactive_form();
    */
};

struct Trailer {
    DocumentCatalog *catalog(Document &document) const;
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
    FREE       = 0,
    NORMAL     = 1,
    COMPRESSED = 2,
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

enum class ChangeSectionType {
    NONE,
    ADDED,
    DELETED,
};

struct ChangeSection {
    ChangeSectionType type = ChangeSectionType::NONE;
    union {
        struct {
            std::string_view deleted_area;
        } deleted;
        struct {
            char *insertion_point;
            char *new_content;
            size_t new_content_length;
        } added = {};
    };
};

struct Document : public ReferenceResolver {
    char *data                              = nullptr;
    int64_t sizeInBytes                     = 0;
    Trailer trailer                         = {};
    CrossReferenceTable crossReferenceTable = {};
    std::vector<IndirectObject *> objects   = {};

    std::vector<ChangeSection> change_sections = {};

    DocumentCatalog *catalog();
    std::vector<Page *> pages();
    size_t page_count();
    std::vector<IndirectObject *> get_all_objects();
    IndirectObject *resolve(const IndirectReference *ref) override;

    bool delete_page(size_t pageNum);

    template <typename T> T *get(Object *object) {
        if (object->is<IndirectReference>()) {
            auto resolvedObject = resolve(object->as<IndirectReference>());
            return resolvedObject->object->as<T>();
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

    [[nodiscard]] bool save_to_file(const std::string &filePath);
    static bool load_from_file(const std::string &filePath, Document &document);

    void delete_raw_section(std::string_view d);
    void add_raw_section(char *insertion_point, char *new_content, size_t new_content_length);

  private:
    IndirectObject *getObject(int64_t objectNumber);
    [[nodiscard]] IndirectObject *loadObject(int64_t objectNumber);

    /**
     * Iterates over all the pages in the document, until 'func' returns 'false'.
     */
    void for_each_page(const std::function<bool(Page *)> &func);
};

} // namespace pdf
