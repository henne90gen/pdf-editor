#pragma once

#include "font.h"
#include "helper/allocator.h"
#include "image.h"
#include "objects.h"
#include "parser.h"

#include <functional>
#include <unordered_set>

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

struct CrossReferenceEntryFree {
    uint64_t nextFreeObjectNumber           = 0;
    uint64_t nextFreeObjectGenerationNumber = 0;
};

struct CrossReferenceEntryNormal {
    uint64_t byteOffset       = 0;
    uint64_t generationNumber = 0;
};

struct CrossReferenceEntryCompressed {
    uint64_t objectNumberOfStream = 0;
    uint64_t indexInStream        = 0;
};

struct CrossReferenceEntry {
    CrossReferenceEntryType type = CrossReferenceEntryType::FREE;
    union {
        CrossReferenceEntryFree free;
        CrossReferenceEntryNormal normal;
        CrossReferenceEntryCompressed compressed = {};
    };
};

struct CrossReferenceTable {
    int64_t firstObjectNumber                = 0;
    int64_t objectCount                      = 0;
    std::vector<CrossReferenceEntry> entries = {};
};

struct Trailer {
    CrossReferenceTable crossReferenceTable = {};
    Dictionary *dict                        = nullptr;
    Stream *stream                          = nullptr;
    Trailer *prev                           = nullptr;

    DocumentCatalog *catalog(Document &document) const;
};

enum class ChangeSectionType {
    NONE,
    ADDED,
    DELETED,
};

struct ChangeSectionDeleted {
    std::string_view deleted_area;
};

struct ChangeSectionAdded {
    const char *insertion_point;
    const char *new_content;
    size_t new_content_length;
};

struct ChangeSection {
    ChangeSectionType type = ChangeSectionType::NONE;
    /// 0 => no associated object, not 0 => the object number of the associated object
    int64_t objectNumber = 0;
    union {
        ChangeSectionAdded added = {};
        ChangeSectionDeleted deleted;
    };

    [[nodiscard]] const char *start_pointer() const;
    [[nodiscard]] size_t size() const;
};

struct Document : public ReferenceResolver {
    Allocator allocator       = {};
    char *data                = nullptr;
    size_t sizeInBytes        = 0;
    int64_t lastCrossRefStart = {};
    Trailer trailer           = {};

    std::unordered_map<uint64_t, IndirectObject *> objectList = {};
    std::vector<ChangeSection> changeSections                 = {};

    virtual ~Document() = default;

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
    /// Iterates over all pages in the document, until 'func' returns 'false'
    void for_each_page(const std::function<ForEachResult(Page *)> &func);
    /// List of objects
    std::vector<IndirectObject *> objects();
    /// Iterates over all objects in the document, until 'func' returns 'false'
    void for_each_object(const std::function<ForEachResult(IndirectObject *)> &func);

    /// Number of indirect objects
    size_t object_count(bool parseObjects = true);
    /// Number of pages
    size_t page_count();
    /// Number of lines
    size_t line_count();
    /// Number of words
    size_t word_count();
    /// Number of characters
    size_t character_count();

    /// Iterates over all images in the document, until 'func' returns 'false'
    void for_each_image(const std::function<ForEachResult(Image &)> &func);

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
    [[maybe_unused]] bool insert_document(Document &otherDocument, size_t atPageNum);
    /// Inserts a file into the pdf document, returns 0 on success
    bool embed_file(const std::string &filePath);

    int64_t next_object_number() const;
    void add_object(int64_t objectNumber, const std::string &content);
    void add_stream(int64_t objectNumber, const std::string &content);
    void replace_object(int64_t objectNumber, const std::string &content);
    void replace_stream(int64_t objectNumber, const std::string &content);

    void delete_raw_section(std::string_view d);
    void add_raw_section(const char *insertion_point, const char *new_content, size_t new_content_length);

    IndirectObject *find_existing_object(Object *object);

  private:
    IndirectObject *get_object(int64_t objectNumber);
    [[nodiscard]] IndirectObject *load_object(int64_t objectNumber);

    bool read_data();
    bool read_trailers(char *crossRefStartPtr, Trailer *currentTrailer);
    bool read_cross_reference_stream(Stream *stream, Trailer *currentTrailer);

    bool write_to_stream(std::ostream &s);
    void write_content(std::ostream &s, char *&ptr, size_t &bytesWrittenUntilXref);
    void write_new_cross_ref_table(std::ostream &s);
    void write_trailer_dict(std::ostream &s, size_t bytesWrittenUntilXref) const;
};

} // namespace pdf
