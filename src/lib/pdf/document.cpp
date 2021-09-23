#include "document.h"

#include <bitset>
#include <fstream>
#include <spdlog/spdlog.h>
#include <sstream>

#include "page.h"

namespace pdf {

IndirectObject *Document::load_object(int64_t objectNumber) {
    auto &entry = crossReferenceTable.entries[objectNumber];
    if (entry.type == CrossReferenceEntryType::FREE) {
        return nullptr;
    }

    if (entry.type == CrossReferenceEntryType::NORMAL) {
        auto start = data + entry.normal.byteOffset;

        // TODO this is dangerous (it might read past the end of the stream)
        size_t length = 0;
        while (std::string_view(start + length, 6) != "endobj") {
            length++;
        }
        length += 6;

        auto input  = std::string_view(start, length);
        auto text   = StringTextProvider(input);
        auto lexer  = TextLexer(text);
        auto parser = Parser(lexer, (ReferenceResolver *)this);
        auto result = parser.parse();
        return result->as<IndirectObject>();
    } else if (entry.type == CrossReferenceEntryType::COMPRESSED) {
        auto streamObject = get_object(entry.compressed.objectNumberOfStream);
        auto stream       = streamObject->object->as<Stream>();
        ASSERT(stream->dictionary->values["Type"]->as<Name>()->value() == "ObjStm");

        auto content       = stream->to_string();
        auto textProvider  = StringTextProvider(content);
        auto lexer         = TextLexer(textProvider);
        auto parser        = Parser(lexer, this);
        int64_t N          = stream->dictionary->values["N"]->as<Integer>()->value;
        auto objectNumbers = std::vector<int64_t>(N);
        for (int i = 0; i < N; i++) {
            auto objNum      = parser.parse()->as<Integer>();
            objectNumbers[i] = objNum->value;
            auto byteOffset  = parser.parse(); // TODO what is this for?
        }
        auto objs = std::vector<Object *>(N);
        for (int i = 0; i < N; i++) {
            auto obj = parser.parse();
            objs[i]  = obj;
        }
        return new IndirectObject(content, objectNumbers[entry.compressed.indexInStream], 0,
                                  objs[entry.compressed.indexInStream]);
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
    for (int i = 0; i < crossReferenceTable.entries.size(); i++) {
        auto &entry = crossReferenceTable.entries[i];
        auto object = get_object(i);
        if (object == nullptr) {
            continue;
        }
        result.push_back(object);
    }
    return result;
}

size_t Document::object_count() {
    size_t result = 0;
    for (int i = 0; i < crossReferenceTable.entries.size(); i++) {
        auto &entry = crossReferenceTable.entries[i];
        if (entry.type == CrossReferenceEntryType::FREE) {
            continue;
        }
        result++;
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

void Document::for_each_page(const std::function<bool(Page *)> &func) {
    auto c            = catalog();
    auto pageTreeRoot = c->page_tree_root(*this);

    if (pageTreeRoot->isPage()) {
        func(new Page(*this, pageTreeRoot));
        return;
    }

    std::vector<PageTreeNode *> queue = {pageTreeRoot};
    while (!queue.empty()) {
        PageTreeNode *current = queue.back();
        queue.pop_back();

        for (auto kid : current->kids()->values) {
            auto resolvedKid = get<PageTreeNode>(kid);
            if (resolvedKid->isPage()) {
                if (!func(new Page(*this, resolvedKid))) {
                    // stop iterating
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
        return true;
    });
    return result;
}

size_t Document::page_count() {
    size_t result = 0;
    for_each_page([&result](auto) {
        result++;
        return true;
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
            return true;
        }

        auto parent = page->node->parent(*this);
        ASSERT(parent != nullptr);
        if (parent->kids()->values.size() == 1) {
            // TODO deal with this case by deleting parent nodes until there are more than one kid
            TODO("deletion of page tree parent nodes is not implemented");
        } else {
            IndirectObject *o = nullptr;
            for (auto obj : objectList) {
                if (obj == nullptr) {
                    continue;
                }
                if (page->node == obj->object) {
                    o = obj;
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

        return false;
    });

    // TODO clean up objects that are no longer required

    return false;
}

void Document::delete_raw_section(std::string_view d) {
    changeSections.push_back({.type = ChangeSectionType::DELETED, .deleted = {.deleted_area = d}});
}

void Document::add_raw_section(char *insertionPoint, char *newContent, size_t newContentLength) {
    changeSections.push_back({.type  = ChangeSectionType::ADDED,
                              .added = {.insertion_point    = insertionPoint,
                                        .new_content        = newContent,
                                        .new_content_length = newContentLength}});
}

DocumentCatalog *Trailer::catalog(Document &document) const {
    Object *root;
    if (dict != nullptr) {
        root = dict->values["Root"];
    } else {
        root = stream->dictionary->values["Root"];
    }
    return document.get<DocumentCatalog>(root);
}

PageTreeNode *DocumentCatalog::page_tree_root(Document &document) {
    return document.get<PageTreeNode>(values["Pages"]);
}

bool Document::insert_document(Document &otherDocument, size_t atPageNum) {
    TODO("implement document insertion");
    return true;
}

} // namespace pdf
