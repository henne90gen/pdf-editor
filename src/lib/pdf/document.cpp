#include "document.h"

#include <bitset>
#include <fstream>
#include <spdlog/spdlog.h>

#include "operator_parser.h"
#include "page.h"

namespace pdf {

IndirectObject *Document::load_object(int64_t objectNumber) {
    CrossReferenceEntry *entry = nullptr;
    for (Trailer *t = &trailer; t != nullptr; t = t->prev) {
        if (objectNumber >= t->crossReferenceTable.firstObjectNumber &&
            objectNumber < t->crossReferenceTable.objectCount) {
            entry = &t->crossReferenceTable.entries[objectNumber - t->crossReferenceTable.firstObjectNumber];
            break;
        }
    }

    if (entry == nullptr) {
        return nullptr;
    }
    if (entry->type == CrossReferenceEntryType::FREE) {
        return nullptr;
    }

    if (entry->type == CrossReferenceEntryType::NORMAL) {
        auto start = data + entry->normal.byteOffset;

        // TODO this is dangerous (it might read past the end of the stream)
        size_t length = 0;
        while (std::string_view(start + length, 6) != "endobj") {
            length++;
        }
        length += 6;

        auto input  = std::string_view(start, length);
        auto text   = StringTextProvider(input);
        auto lexer  = TextLexer(text);
        auto parser = Parser(lexer, allocator, this);
        auto result = parser.parse();
        ASSERT(result != nullptr);
        return result->as<IndirectObject>();
    } else if (entry->type == CrossReferenceEntryType::COMPRESSED) {
        auto streamObject = get_object(entry->compressed.objectNumberOfStream);
        auto stream       = streamObject->object->as<Stream>();
        ASSERT(stream->dictionary->must_find<Name>("Type")->value() == "ObjStm");

        auto content      = stream->decode(allocator);
        auto textProvider = StringTextProvider(content);
        auto lexer        = TextLexer(textProvider);
        auto parser       = Parser(lexer, allocator, this);
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

        return allocator.allocate<IndirectObject>(content, objectNumbers[entry->compressed.indexInStream], 0,
                                                  objs[entry->compressed.indexInStream]);
    }
    ASSERT(false);
}

IndirectObject *Document::get_object(int64_t objectNumber) {
    if (objectList[objectNumber] != nullptr) {
        return objectList[objectNumber];
    }

    auto object              = load_object(objectNumber);
    objectList[objectNumber] = object;
    return object;
}

IndirectObject *Document::resolve(const IndirectReference *ref) { return get_object(ref->objectNumber); }

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
    for (int64_t i = 0; i < static_cast<int64_t>(trailer.crossReferenceTable.entries.size()); i++) {
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
        for (auto &entry : trailer.crossReferenceTable.entries) {
            if (entry.type == CrossReferenceEntryType::FREE) {
                continue;
            }
            result++;
        }
    }
    return result;
}

PageTreeNode *PageTreeNode::parent(Document &document) {
    auto opt = values.find("Parent");
    if (!opt.has_value()) {
        return nullptr;
    }
    return document.get<PageTreeNode>(opt.value());
}

void Document::for_each_page(const std::function<ForEachResult(Page *)> &func) {
    auto c            = catalog();
    auto pageTreeRoot = c->page_tree_root(*this);
    if (pageTreeRoot == nullptr) {
        spdlog::warn("Document did not have any pages");
        return;
    }

    if (pageTreeRoot->is_page()) {
        func(allocator.allocate<Page>(*this, pageTreeRoot));
        return;
    }

    std::vector<PageTreeNode *> queue = {pageTreeRoot};
    while (!queue.empty()) {
        PageTreeNode *current = queue.back();
        queue.pop_back();

        for (auto kid : current->kids()->values) {
            auto resolvedKid = get<PageTreeNode>(kid);
            if (resolvedKid->is_page()) {
                Page *page           = allocator.allocate<Page>(*this, resolvedKid);
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

DocumentCatalog *Document::catalog() { return trailer.catalog(*this); }

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

bool Document::delete_page(size_t pageNum) {
    auto count = page_count();
    if (count == 1) {
        spdlog::warn("Cannot delete last page of document");
        return true;
    }

    if (pageNum < 1 || pageNum > count) {
        spdlog::warn("Tried to delete page {}, which is outside of the inclusive range [1, {}]", pageNum, count);
        return true;
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

    return false;
}

void Document::delete_raw_section(std::string_view d) {
    changeSections.push_back({.type = ChangeSectionType::DELETED, .deleted = {.deleted_area = d}});
}

void Document::add_raw_section(const char *insertionPoint, const char *newContent, size_t newContentLength) {
    changeSections.push_back({.type  = ChangeSectionType::ADDED,
                              .added = {.insertion_point    = insertionPoint,
                                        .new_content        = newContent,
                                        .new_content_length = newContentLength}});
}

DocumentCatalog *Trailer::catalog(Document &document) const {
    std::optional<Object *> opt;
    if (dict != nullptr) {
        opt = dict->values.find("Root");
    } else {
        opt = stream->dictionary->values.find("Root");
    }
    ASSERT(opt.has_value());
    return document.get<DocumentCatalog>(opt.value());
}

PageTreeNode *DocumentCatalog::page_tree_root(Document &document) {
    auto opt = values.find("Pages");
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
    for_each_object([this, &func](IndirectObject *obj) {
        if (!obj->object->is<Stream>()) {
            return ForEachResult::CONTINUE;
        }

        const auto stream  = obj->object->as<Stream>();
        const auto typeOpt = stream->dictionary->find<Name>("Type");
        if (!typeOpt.has_value() || typeOpt.value()->value() != "XObject") {
            return ForEachResult::CONTINUE;
        }

        const auto subtypeOpt = stream->dictionary->find<Name>("Subtype");
        if (!subtypeOpt.has_value() || subtypeOpt.value()->value() != "Image") {
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

        Image image = {
              .allocator        = allocator,
              .width            = widthOpt.value()->value,
              .height           = heightOpt.value()->value,
              .bitsPerComponent = bitsPerComponentOpt.value()->value,
              .stream           = stream,
        };
        return func(image);
    });
}

} // namespace pdf
