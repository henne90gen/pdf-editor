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
    int64_t firstObjectNumber                = 0;
    int64_t objectCount                      = 0;
    std::vector<CrossReferenceEntry> entries = {};
};

struct Trailer {
    Dictionary *dict                        = nullptr;
    CrossReferenceTable crossReferenceTable = {};
    Stream *stream                          = nullptr;
    Trailer *prev                           = nullptr;

    DocumentCatalog *catalog(Document &document) const;
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
    char *data                = nullptr;
    size_t sizeInBytes        = 0;
    int64_t lastCrossRefStart = {};
    Trailer trailer           = {};

    std::unordered_map<uint64_t, IndirectObject *> objectList = {};
    std::vector<ChangeSection> changeSections                 = {};

    template <typename T> T *get(Object *object) {
        if (object->is<IndirectReference>()) {
            auto resolvedObject = resolve(object->as<IndirectReference>());
            ASSERT(resolvedObject != nullptr);
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

    IndirectObject *resolve(const IndirectReference *ref) override;

    /// The document catalog of this document
    DocumentCatalog *catalog();
    /// List of pages
    std::vector<Page *> pages();
    /// List of objects
    std::vector<IndirectObject *> objects();

    /// Number of indirect objects
    size_t object_count();
    /// Number of pages
    size_t page_count();
    /// Number of lines
    size_t line_count();
    /// Number of words
    size_t word_count();
    /// Number of characters
    size_t character_count();

    /// Writes the PDF-document to the given filePath, returns 0 on success
    [[nodiscard]] bool write_to_file(const std::string &filePath);
    /// Writes the PDF-document to a newly allocated buffer, returns 0 on success
    [[nodiscard]] bool write_to_memory(char *&buffer, size_t &size);
    /// Reads the PDF-document specified by the given filePath, returns 0 on success
    static bool read_from_file(const std::string &filePath, Document &document);
    /// Reads the PDF-document from the given buffer, returns 0 on success
    static bool read_from_memory(char *buffer, size_t size, Document &document);

    /// Deletes the page with the given page number, returns 0 on success
    bool delete_page(size_t pageNum);
    /// Insert another document into this one so that the first page of the inserted document has the given page number
    bool insert_document(Document &otherDocument, size_t atPageNum);

    void delete_raw_section(std::string_view d);
    void add_raw_section(char *insertion_point, char *new_content, size_t new_content_length);

  private:
    IndirectObject *get_object(int64_t objectNumber);
    [[nodiscard]] IndirectObject *load_object(int64_t objectNumber);

    bool read_data();
    bool read_trailers(char *crossRefStartPtr, Trailer *currentTrailer);
    bool read_cross_reference_stream(Stream *stream, Trailer *currentTrailer);

    bool write_to_stream(std::ostream &s);
    void write_content(std::ostream &s, char *&ptr, size_t &bytesWrittenUntilXref);
    void write_new_cross_ref_table(std::ostream &s);
    void write_trailer_dict(std::ostream &s, size_t bytesWrittenUntilXref);

    /// Iterates over all the pages in the document, until 'func' returns 'false'.
    void for_each_page(const std::function<bool(Page *)> &func);
};

} // namespace pdf
