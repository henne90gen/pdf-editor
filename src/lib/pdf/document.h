#pragma once

#include <functional>
#include <unordered_set>

#include "pdf/font.h"
#include "pdf/image.h"
#include "pdf/memory/arena_allocator.h"
#include "pdf/objects.h"
#include "pdf/parser.h"

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
    IndirectObject *streamObject            = nullptr;
    Trailer *prev                           = nullptr;
};

struct ObjectMetadata {
    std::string_view data = {};
    bool isInObjectStream = false;
};

struct DocumentFileMetadata {
    std::unordered_map<IndirectObject *, ObjectMetadata> objects = {};
    std::unordered_map<Trailer *, std::string_view> trailers     = {};
};

struct DocumentFile {
    std::string path;
    uint8_t *data                 = nullptr;
    size_t sizeInBytes            = 0;
    int64_t lastCrossRefStart     = {};
    Trailer trailer               = {};
    DocumentFileMetadata metadata = {};

    /// Points to the byte right after the end of the data buffer
    uint8_t *end_ptr() const { return data + sizeInBytes; }
    /// Returns true if the given pointer is outside of the range of the data buffer
    bool is_out_of_range(const uint8_t *ptr) const { return ptr < data || ptr >= end_ptr(); }
    bool is_out_of_range(const uint8_t *ptr, size_t size) const { return ptr < data || ptr + size >= end_ptr(); }
    bool is_out_of_range(std::string_view sv) const {
        return reinterpret_cast<const uint8_t *>(sv.data()) < data ||
               reinterpret_cast<const uint8_t *>(sv.data()) + sv.size() >= end_ptr();
    }
};

struct ReadMetadata {
    std::vector<std::string_view> trailers                 = {};
    std::unordered_map<Object *, std::string_view> objects = {};
};

struct Document : public ReferenceResolver {
    Allocator allocator                                       = {};
    DocumentFile file                                         = {};
    std::unordered_map<uint64_t, IndirectObject *> objectList = {};

    ~Document();

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
    /// Iterates over all pages in the document
    void for_each_page(const std::function<ForEachResult(Page *)> &func);
    /// List of objects
    std::vector<IndirectObject *> objects();
    /// Iterates over all objects in the document
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

    /// Iterates over all images in the document
    void for_each_image(const std::function<ForEachResult(Image &)> &func);
    /// Iterates over all embedded files in the document
    void for_each_embedded_file(const std::function<ForEachResult(EmbeddedFile *)> &func);

    /// Writes the PDF-document to the given filePath
    [[nodiscard]] Result write_to_file(const std::string &filePath);
    /// Writes the PDF-document to a newly allocated buffer
    /// TODO think about who owns the memory
    [[nodiscard]] Result write_to_memory(uint8_t *&buffer, size_t &size);
    /// Reads the PDF-document specified by the given filePath
    static ValueResult<Document> read_from_file(const std::string &filePath, bool loadAllObjects = true);
    /// Reads the PDF-document from the given buffer
    static ValueResult<Document> read_from_memory(const uint8_t *buffer, size_t size, bool loadAllObjects = true);

    // Deletes the page with the given page number
    Result delete_page(size_t pageNum);
    // Insert another document into this one so that the first page of the inserted document has the given page number
    [[maybe_unused]] bool insert_document(Document &otherDocument, size_t atPageNum);
    // Inserts a file into the pdf document
    Result embed_file(const std::string &filePath);

    int64_t next_object_number() const;
    int64_t add_object(Object *object);

    // Finds the IndirectObject that wraps the given Object
    IndirectObject *find_existing_object(Object *object);

    // releases all memory that was allocated by this document instance
    void destroy() { allocator.destroy(); }

  private:
    int64_t currentResolutionObjectNumber = 0;
    DocumentCatalog *cachedRoot           = nullptr;
    std::vector<Page *> cachedPages       = {};

    IndirectObject *get_object(int64_t objectNumber);
    [[nodiscard]] std::pair<IndirectObject *, std::string_view> load_object(int64_t objectNumber);
};

} // namespace pdf
