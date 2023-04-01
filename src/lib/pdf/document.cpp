#include "document.h"

#include <bitset>
#include <fstream>
#include <spdlog/spdlog.h>
#include <sstream>
#include <zlib.h>

#include "pdf/hash/hex_string.h"
#include "pdf/hash/md5.h"
#include "pdf/operator_parser.h"
#include "pdf/page.h"

namespace pdf {

Document::~Document() {
    bool isFirst        = true;
    auto currentTrailer = &file.trailer;
    while (currentTrailer != nullptr) {
        auto nextTrailer = currentTrailer->prev;

        if (!isFirst) {
            currentTrailer->~Trailer();
        }

        currentTrailer = nextTrailer;
        isFirst        = false;
    }
}

// TODO return a ValueResult instead, for better error messages
std::pair<IndirectObject *, std::string_view> Document::load_object(int64_t objectNumber) {
    CrossReferenceEntry *entry = nullptr;
    for (Trailer *t = &file.trailer; t != nullptr; t = t->prev) {
        if (objectNumber >= t->crossReferenceTable.firstObjectNumber &&
            objectNumber < t->crossReferenceTable.objectCount) {
            entry = &t->crossReferenceTable.entries[objectNumber - t->crossReferenceTable.firstObjectNumber];
            break;
        }
    }

    if (entry == nullptr) {
        return {nullptr, {}};
    }
    if (entry->type == CrossReferenceEntryType::FREE) {
        return {nullptr, {}};
    }

    if (entry->type == CrossReferenceEntryType::NORMAL) {
        auto start = reinterpret_cast<char *>(file.data + entry->normal.byteOffset);
        if (file.is_out_of_range(std::string_view(start, 3))) {
            return {nullptr, {}};
        }

        size_t length = 0;
        while (std::string_view(start + length, 6) != "endobj") {
            length++;
            if (file.is_out_of_range(std::string_view(start + length, 6))) {
                // return Result::error("Unexpectedly reached end of file");
                return {nullptr, {}};
            }
        }
        length += 6;

        auto input  = std::string_view(start, length);
        auto text   = StringTextProvider(input);
        auto lexer  = TextLexer(text);
        auto parser = Parser(lexer, allocator.arena(), this);
        auto result = parser.parse();
        if (result == nullptr || !result->is<IndirectObject>()) {
            return {nullptr, {}};
        }
        return {result->as<IndirectObject>(), input};
    } else if (entry->type == CrossReferenceEntryType::COMPRESSED) {
        auto streamObject = get_object(entry->compressed.objectNumberOfStream);
        auto stream       = streamObject->object->as<Stream>();
        ASSERT(stream->dictionary->must_find<Name>("Type")->value == "ObjStm");

        auto content      = stream->decode(allocator);
        auto textProvider = StringTextProvider(content);
        auto lexer        = TextLexer(textProvider);
        auto parser       = Parser(lexer, allocator.arena(), this);
        int64_t N         = stream->dictionary->must_find<Integer>("N")->value;

        // TODO cache objectNumbers and corresponding objects
        auto objectNumbers = std::vector<int64_t>(N);
        for (int i = 0; i < N; i++) {
            auto objNum      = parser.parse()->as<Integer>();
            objectNumbers[i] = objNum->value;
            // parse the byteOffset as well
            parser.parse();
        }

        auto objs = std::vector<Object *>(N);
        for (int i = 0; i < N; i++) {
            auto obj = parser.parse();
            objs[i]  = obj;
        }

        auto object = allocator.arena().push<IndirectObject>(objectNumbers[entry->compressed.indexInStream], 0,
                                                             objs[entry->compressed.indexInStream]);
        // TODO the content does not refer to the original PDF document, but instead to a decoded stream
        return {object, content};
    }
    ASSERT(false);
}

IndirectObject *Document::get_object(int64_t objectNumber) {
    if (objectList[objectNumber] != nullptr) {
        return objectList[objectNumber];
    }

    auto object                         = load_object(objectNumber);
    objectList[objectNumber]            = object.first;
    file.metadata.objects[object.first] = {object.second,
                                           false}; // TODO determine whether this obj is from an object stream or not
    return object.first;
}

IndirectObject *Document::resolve(const IndirectReference *ref) {
    if (currentResolutionObjectNumber == ref->objectNumber) {
        return nullptr;
    }

    // TODO this is not enough to protect against stack overflows
    currentResolutionObjectNumber = ref->objectNumber;
    auto result                   = get_object(ref->objectNumber);
    currentResolutionObjectNumber = 0;

    return result;
}

std::vector<IndirectObject *> Document::objects() {
    std::vector<IndirectObject *> result = {};
    for_each_object([&result](IndirectObject *obj) {
        result.push_back(obj);
        return ForEachResult::CONTINUE;
    });
    return result;
}

void Document::for_each_object(const std::function<ForEachResult(IndirectObject *)> &func) {
    // FIXME this does not consider objects from 'Prev'-ious trailers
    for (int64_t i = 0; i < static_cast<int64_t>(file.trailer.crossReferenceTable.entries.size()); i++) {
        auto object = get_object(i);
        if (object == nullptr) {
            continue;
        }

        ForEachResult result = func(object);
        if (result == ForEachResult::BREAK) {
            break;
        }
    }
}

size_t Document::object_count(const bool parseObjects) {
    size_t result = 0;
    if (parseObjects) {
        for_each_object([&result](IndirectObject *) {
            result++;
            return ForEachResult::CONTINUE;
        });
    } else {
        for (auto &entry : file.trailer.crossReferenceTable.entries) {
            if (entry.type == CrossReferenceEntryType::FREE) {
                continue;
            }
            result++;
        }
    }
    return result;
}

PageTreeNode *PageTreeNode::parent(Document &document) {
    auto itr = values.find("Parent");
    if (itr == values.end()) {
        return nullptr;
    }
    return document.get<PageTreeNode>(itr->second);
}

void Document::for_each_page(const std::function<ForEachResult(Page *)> &func) {
    auto c            = catalog();
    auto pageTreeRoot = c->page_tree_root(*this);
    if (pageTreeRoot == nullptr) {
        spdlog::warn("Document did not have any pages");
        return;
    }

    // FIXME cache page objects (otherwise they are re-created each time the pages are iterated)

    if (pageTreeRoot->is_page()) {
        func(allocator.arena().push<Page>(*this, pageTreeRoot));
        return;
    }

    std::vector<PageTreeNode *> queue = {pageTreeRoot};
    while (!queue.empty()) {
        PageTreeNode *current = queue.back();
        queue.pop_back();

        for (auto kid : current->kids()->values) {
            auto resolvedKid = get<PageTreeNode>(kid);
            if (resolvedKid->is_page()) {
                Page *page           = allocator.arena().push<Page>(*this, resolvedKid);
                ForEachResult result = func(page);
                if (result == ForEachResult::BREAK) {
                    return;
                }
            } else {
                queue.push_back(resolvedKid);
            }
        }
    }
}

DocumentCatalog *Document::catalog() {
    if (rootCache != nullptr) {
        return rootCache;
    }

    Object *obj;
    if (file.trailer.dict != nullptr) {
        obj = file.trailer.dict->must_find<Object>("Root");
    } else {
        obj = file.trailer.streamObject->object->as<Stream>()->dictionary->must_find<Object>("Root");
    }
    ASSERT(obj != nullptr);

    rootCache = get<DocumentCatalog>(obj);
    return rootCache;
}

std::vector<Page *> Document::pages() {
    auto result = std::vector<Page *>();
    for_each_page([&result](auto page) {
        result.push_back(page);
        return ForEachResult::CONTINUE;
    });
    return result;
}

size_t Document::page_count() {
    size_t result = 0;
    for_each_page([&result](auto) {
        result++;
        return ForEachResult::CONTINUE;
    });
    return result;
}

Result Document::delete_page(size_t pageNum) {
    auto count = page_count();
    if (count == 1) {
        return Result::error("Cannot delete last page of document");
    }

    if (pageNum < 1 || pageNum > count) {
        return Result::error("Tried to delete page {}, which is outside of the inclusive range [1, {}]", pageNum,
                             count);
    }

    size_t currentPageNum = 1;
    for_each_page([&currentPageNum, &pageNum, this](Page *page) {
        if (currentPageNum != pageNum) {
            currentPageNum++;
            return ForEachResult::CONTINUE;
        }

        auto parent = page->node->parent(*this);
        ASSERT(parent != nullptr);
        if (parent->kids()->values.size() == 1) {
            // TODO deal with this case by deleting parent nodes until there are more than one kid
        } else {
            IndirectObject *o = nullptr;
            for (auto obj : objectList) {
                if (obj.second == nullptr) {
                    continue;
                }
                if (page->node == obj.second->object) {
                    o = obj.second;
                    break;
                }
            }
            ASSERT(o != nullptr);

            size_t childToDeleteIndex = 0;
            for (auto kid : parent->kids()->values) {
                // TODO should the generation number also be compared?
                if (kid->as<IndirectReference>()->objectNumber == o->objectNumber) {
                    break;
                }
                childToDeleteIndex++;
            }
            parent->kids()->remove_element(*this, childToDeleteIndex);
            parent->count()->set(*this, static_cast<int64_t>(parent->kids()->values.size()));
        }

        return ForEachResult::BREAK;
    });

    // TODO clean up objects that are no longer required

    return Result::ok();
}

PageTreeNode *DocumentCatalog::page_tree_root(Document &document) {
    auto opt = find<Object>("Pages");
    if (!opt.has_value()) {
        return nullptr;
    }
    return document.get<PageTreeNode>(opt.value());
}

[[maybe_unused]] bool Document::insert_document(Document & /*otherDocument*/, size_t /*atPageNum*/) {
    // TODO implement document insertion
    return true;
}

size_t Document::line_count() {
    // TODO implement line count
    return 0;
}

size_t Document::word_count() {
    // TODO implement word count
    return 0;
}

size_t Document::character_count() {
    size_t result = 0;
    for_each_page([&result](Page *page) {
        result += page->character_count();
        return ForEachResult::CONTINUE;
    });
    return result;
}

void Document::for_each_image(const std::function<ForEachResult(Image &)> &func) {
    for_each_object([&func](IndirectObject *obj) {
        if (!obj->object->is<Stream>()) {
            return ForEachResult::CONTINUE;
        }

        const auto stream  = obj->object->as<Stream>();
        const auto typeOpt = stream->dictionary->find<Name>("Type");
        if (!typeOpt.has_value() || typeOpt.value()->value != "XObject") {
            return ForEachResult::CONTINUE;
        }

        const auto subtypeOpt = stream->dictionary->find<Name>("Subtype");
        if (!subtypeOpt.has_value() || subtypeOpt.value()->value != "Image") {
            return ForEachResult::CONTINUE;
        }

        const auto widthOpt = stream->dictionary->find<Integer>("Width");
        if (!widthOpt.has_value()) {
            return ForEachResult::CONTINUE;
        }

        const auto heightOpt = stream->dictionary->find<Integer>("Height");
        if (!heightOpt.has_value()) {
            return ForEachResult::CONTINUE;
        }

        const auto bitsPerComponentOpt = stream->dictionary->find<Integer>("BitsPerComponent");
        if (!bitsPerComponentOpt.has_value()) {
            return ForEachResult::CONTINUE;
        }

        auto image             = Image();
        image.width            = widthOpt.value()->value;
        image.height           = heightOpt.value()->value;
        image.bitsPerComponent = bitsPerComponentOpt.value()->value;
        image.stream           = stream;
        return func(image);
    });
}

ValueResult<Stream *> create_embedded_file_stream(Allocator &allocator, const std::string &filePath) {
    auto is = std::ifstream();
    is.open(filePath, std::ios::in | std::ifstream::ate | std::ios::binary);

    if (!is.is_open()) {
        return ValueResult<Stream *>::error("Failed to open file for reading: '{}'", filePath);
    }

    auto fileName   = filePath.substr(filePath.find_last_of("/\\") + 1);
    size_t fileSize = is.tellg();
    is.seekg(0);
    spdlog::info("Embedding {} bytes of file '{}'", fileSize, filePath);

    auto tempArena = allocator.temporary();
    auto fileData  = tempArena.arena().push(fileSize);
    is.read((char *)fileData, static_cast<std::streamsize>(fileSize));

    auto checksum    = hash::md5_checksum(reinterpret_cast<const uint8_t *>(fileData), fileSize);
    auto checksumStr = hash::to_hex_string(checksum);
    std::unordered_map<std::string, Object *> params = {
          {"Size", allocator.arena().push<Integer>(fileSize)},
          // TODO add CreationDate
          // TODO add ModDate
          {"CheckSum", allocator.arena().push<LiteralString>(checksumStr)},
    };
    std::unordered_map<std::string, Object *> dict = {
          {"Type", allocator.arena().push<Name>("EmbeddedFile")},
          // {"SubType", MIME type}, // TODO parse MIME type and add as SubType
          {"Params", allocator.arena().push<Dictionary>(params)},
    };

    auto result = Stream::create_from_unencoded_data(allocator, dict, std::string_view((char *)fileData, fileSize));
    return ValueResult<Stream *>::ok(result);
}

Result Document::embed_file(const std::string &filePath) {
    auto result = create_embedded_file_stream(allocator, filePath);
    if (result.has_error()) {
        return result.drop_value();
    }

    add_object(result.value());

    return Result::ok();
}

int64_t Document::add_object(Object *object) {
    auto objectNumber        = next_object_number();
    objectList[objectNumber] = allocator.arena().push<IndirectObject>(objectNumber, 0, object);
    return objectNumber;
}

int64_t Document::next_object_number() const { return objectList.size(); }

IndirectObject *Document::find_existing_object(Object *object) {
    for (auto &entry : objectList) {
        if (entry.second->object == object) {
            return entry.second;
        }
    }
    return nullptr;
}

void Document::for_each_embedded_file(const std::function<ForEachResult(EmbeddedFile *)> &func) {
    // TODO iterate file specifications instead and pass file name and EmbeddedFile to func

    for_each_object([&func](pdf::IndirectObject *obj) {
        if (!obj->object->is<Stream>()) {
            return ForEachResult::CONTINUE;
        }

        const auto stream   = obj->object->as<Stream>();
        const auto dictType = stream->dictionary->find<Name>("Type");
        if (!dictType.has_value()) {
            return ForEachResult::CONTINUE;
        }
        if (dictType.value()->value != "EmbeddedFile") {
            return ForEachResult::CONTINUE;
        }

        func(stream->as<EmbeddedFile>());

        return ForEachResult::CONTINUE;
    });
}

} // namespace pdf
