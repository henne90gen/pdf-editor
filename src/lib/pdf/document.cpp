#include "document.h"

#include <bitset>
#include <fstream>
#include <spdlog/spdlog.h>

#include "page.h"

namespace pdf {

IndirectObject *Document::loadObject(int64_t objectNumber) {
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
    } else {
        auto stream = getObject(entry.compressed.objectNumberOfStream)->object->as<Stream>();
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
}

IndirectObject *Document::getObject(int64_t objectNumber) {
    if (objects[objectNumber] != nullptr) {
        return objects[objectNumber];
    }

    auto object           = loadObject(objectNumber);
    objects[objectNumber] = object;
    return object;
}

IndirectObject *Document::resolve(const IndirectReference *ref) { return getObject(ref->objectNumber); }

std::vector<IndirectObject *> Document::get_all_objects() {
    std::vector<IndirectObject *> result = {};
    for (int i = 0; i < crossReferenceTable.entries.size(); i++) {
        auto &entry = crossReferenceTable.entries[i];
        auto object = getObject(i);
        if (object == nullptr) {
            continue;
        }
        result.push_back(object);
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

Dictionary *parseDict(char *start, size_t length) {
    ASSERT(start != nullptr);
    ASSERT(length > 0);
    const std::string_view input = std::string_view(start, length);
    auto text                    = StringTextProvider(input);
    auto lexer                   = TextLexer(text);
    auto parser                  = Parser(lexer);
    auto result                  = parser.parse();
    return result->as<Dictionary>();
}

Stream *parseStream(char *start, size_t length) {
    ASSERT(start != nullptr);
    ASSERT(length > 0);
    const std::string_view input = std::string_view(start, length);
    auto text                    = StringTextProvider(input);
    auto lexer                   = TextLexer(text);
    auto parser                  = Parser(lexer);
    auto result                  = parser.parse();
    return result->as<IndirectObject>()->object->as<Stream>();
}

bool readTrailer(Document &file) {
    size_t eofMarkerLength = 5;
    char *eofMarkerStart   = file.data + (file.sizeInBytes - eofMarkerLength);
    if (file.data[file.sizeInBytes - 1] == '\n') {
        eofMarkerStart -= 1;
    }
    if (std::string(eofMarkerStart, eofMarkerLength) != "%%EOF") {
        spdlog::error("Last line did not have '%%EOF'");
        return false;
    }

    char *lastCrossRefStartPtr = eofMarkerStart - 2;
    while (*lastCrossRefStartPtr != '\n' && file.data < lastCrossRefStartPtr) {
        lastCrossRefStartPtr--;
    }
    if (file.data == lastCrossRefStartPtr) {
        spdlog::error("Unexpectedly reached start of file");
        return false;
    }
    lastCrossRefStartPtr++;

    try {
        file.trailer.lastCrossRefStart =
              std::stoll(std::string(lastCrossRefStartPtr, eofMarkerStart - 1 - lastCrossRefStartPtr));
    } catch (std::invalid_argument &err) {
        spdlog::error("Failed to parse byte offset of cross reference table: {}", err.what());
        return false;
    } catch (std::out_of_range &err) {
        spdlog::error("Failed to parse byte offset of cross reference table: {}", err.what());
        return false;
    }

    const auto xrefKeyword = std::string_view(file.data + file.trailer.lastCrossRefStart, 4);
    if (xrefKeyword != "xref") {
        char *startxrefPtr    = lastCrossRefStartPtr - 10;
        auto startOfStream    = file.data + file.trailer.lastCrossRefStart;
        size_t lengthOfStream = startxrefPtr - startOfStream;
        file.trailer.set_stream(parseStream(startOfStream, lengthOfStream));
        return true;
    } else {
        char *startxrefPtr = lastCrossRefStartPtr - 10;
        auto startxrefLine = std::string_view(startxrefPtr, 9);
        if (startxrefLine == "tartxref\r") {
            startxrefPtr -= 1;
            startxrefLine = std::string_view(startxrefPtr, 9);
        }
        if (startxrefLine != "startxref") {
            spdlog::error("Expected 'startxref', but got '{}'", startxrefLine);
            return false;
        }

        char *startOfTrailerPtr = startxrefPtr;
        while (std::string_view(startOfTrailerPtr, 7) != "trailer" && file.data < startOfTrailerPtr) {
            startOfTrailerPtr--;
        }
        if (file.data == startOfTrailerPtr) {
            spdlog::error("Unexpectedly reached start of file");
            return false;
        }

        startOfTrailerPtr += 8;
        if (*(startxrefPtr - 1) == '\n') {
            startxrefPtr--;
        }
        if (*(startxrefPtr - 1) == '\r') {
            startxrefPtr--;
        }
        auto lengthOfTrailerDict = startxrefPtr - startOfTrailerPtr;
        file.trailer.set_dict(parseDict(startOfTrailerPtr, lengthOfTrailerDict));
        return true;
    }
}

bool readCrossReferenceTable(Document &file) {
    // exactly one of 'stream' or 'dict' has to be non-null
    ASSERT(file.trailer.get_stream() != nullptr || file.trailer.get_dict() != nullptr);
    ASSERT(file.trailer.get_stream() == nullptr || file.trailer.get_dict() == nullptr);

    if (file.trailer.get_dict() != nullptr) {
        char *crossRefPtr = file.data + file.trailer.lastCrossRefStart;
        if (std::string(crossRefPtr, 4) != "xref") {
            std::cerr << "Expected xref" << std::endl;
            return false;
        }
        crossRefPtr += 4;
        if (*crossRefPtr == '\r') {
            crossRefPtr++;
        }
        if (*crossRefPtr == '\n') {
            crossRefPtr++;
        }

        int64_t spaceLocation = -1;
        char *tmp             = crossRefPtr;
        while (*tmp != '\n') {
            if (*tmp == ' ') {
                spaceLocation = tmp - crossRefPtr;
            }
            tmp++;
        }
        auto beginTable = tmp + 1;
        auto metaData   = std::string(crossRefPtr, (beginTable)-crossRefPtr);
        // TODO catch exceptions
        file.crossReferenceTable.firstObjectId = std::stoll(metaData.substr(0, spaceLocation));
        file.crossReferenceTable.objectCount   = std::stoll(metaData.substr(spaceLocation));
        for (int i = 0; i < file.crossReferenceTable.objectCount; i++) {
            // nnnnnnnnnn ggggg f__
            auto s = std::string(beginTable, 20);
            // TODO catch exceptions
            uint64_t num0 = std::stoll(s.substr(0, 10));
            uint64_t num1 = std::stoll(s.substr(11, 16));

            CrossReferenceEntry entry = {};
            if (s[17] == 'f') {
                entry.type                                = CrossReferenceEntryType ::FREE;
                entry.free.nextFreeObjectNumber           = num0;
                entry.free.nextFreeObjectGenerationNumber = num1;
            } else {
                entry.type                    = CrossReferenceEntryType::NORMAL;
                entry.normal.byteOffset       = num0;
                entry.normal.generationNumber = num1;
            }

            file.crossReferenceTable.entries.push_back(entry);
            beginTable += 20;
        }
    } else {
        Stream *stream  = file.trailer.get_stream();
        auto W          = stream->dictionary->values["W"]->as<Array>();
        auto sizeField0 = W->values[0]->as<Integer>()->value;
        auto sizeField1 = W->values[1]->as<Integer>()->value;
        auto sizeField2 = W->values[2]->as<Integer>()->value;
        auto content    = stream->to_string();
        auto contentPtr = content.data();

        // verify that the content of the stream matches the size in the dictionary
        ASSERT(stream->dictionary->values["Size"]->as<Integer>()->value ==
               content.size() / (sizeField0 + sizeField1 + sizeField2));

        for (size_t i = 0; i < content.size(); i += sizeField0 + sizeField1 + sizeField2) {
            auto tmp      = contentPtr + i;
            uint64_t type = 0;
            if (sizeField0 == 0) {
                type = 1; // default value for type
            }
            for (int j = 0; j < sizeField0; j++) {
                uint8_t c      = *(tmp + j);
                uint64_t shift = (sizeField0 - (j + 1)) * 8;
                type |= c << shift;
            }

            uint64_t field1 = 0;
            for (int j = 0; j < sizeField1; j++) {
                uint8_t c      = *(tmp + j + sizeField0);
                uint64_t shift = (sizeField1 - (j + 1)) * 8;
                field1 |= c << shift;
            }

            uint64_t field2 = 0;
            for (int j = 0; j < sizeField2; j++) {
                uint8_t c      = *(tmp + j + sizeField0 + sizeField1);
                uint64_t shift = (sizeField2 - (j + 1)) * 8;
                field2 |= c << shift;
            }

            switch (type) {
            case 0: {
                CrossReferenceEntry entry                 = {};
                entry.type                                = CrossReferenceEntryType::FREE;
                entry.free.nextFreeObjectNumber           = field1;
                entry.free.nextFreeObjectGenerationNumber = field2;
                file.crossReferenceTable.entries.push_back(entry);
            } break;
            case 1: {
                CrossReferenceEntry entry     = {};
                entry.type                    = CrossReferenceEntryType::NORMAL;
                entry.normal.byteOffset       = field1;
                entry.normal.generationNumber = field2;
                file.crossReferenceTable.entries.push_back(entry);
            } break;
            case 2: {
                CrossReferenceEntry entry             = {};
                entry.type                            = CrossReferenceEntryType::COMPRESSED;
                entry.compressed.objectNumberOfStream = field1;
                entry.compressed.indexInStream        = field2;
                file.crossReferenceTable.entries.push_back(entry);
            } break;
            default:
                spdlog::warn("Encountered unknown cross reference stream entry field type: {}", type);
                break;
            }
        }
    }

    file.objects.resize(file.crossReferenceTable.entries.size(), nullptr);
    return true;
}

bool Document::load_from_file(const std::string &filePath, Document &document) {
    auto is = std::ifstream();
    is.open(filePath, std::ios::in | std::ifstream::ate | std::ios::binary);

    if (!is.is_open()) {
        spdlog::error("Failed to open pdf file for reading");
        return false;
    }

    document.sizeInBytes = is.tellg();
    document.data        = (char *)malloc(document.sizeInBytes);

    is.seekg(0);
    is.read(document.data, document.sizeInBytes);
    is.close();

    if (!readTrailer(document)) {
        return false;
    }
    if (!readCrossReferenceTable(document)) {
        return false;
    }

    return true;
}

bool Document::save_to_file(const std::string &filePath) {
    auto os = std::ofstream();
    os.open(filePath, std::ios::out | std::ios::binary);

    if (!os.is_open()) {
        spdlog::error("Failed to open pdf file for writing");
        return false;
    }

    if (change_sections.empty()) {
        os.write(data, sizeInBytes);
        os.close();
        return true;
    }

    std::sort(change_sections.begin(), change_sections.end(), [](const ChangeSection &a, const ChangeSection &b) {
        if (a.type == ChangeSectionType::ADDED && b.type == ChangeSectionType::ADDED) {
            return a.added.insertion_point < b.added.insertion_point;
        }
        if (a.type == ChangeSectionType::ADDED && b.type == ChangeSectionType::DELETED) {
            return a.added.insertion_point < b.deleted.deleted_area.data();
        }
        if (a.type == ChangeSectionType::DELETED && b.type == ChangeSectionType::ADDED) {
            return a.deleted.deleted_area.data() < b.added.insertion_point;
        }
        if (a.type == ChangeSectionType::DELETED && b.type == ChangeSectionType::DELETED) {
            return a.deleted.deleted_area.data() < b.deleted.deleted_area.data();
        }
        ASSERT(false);
    });

    auto ptr = data;
    for (const auto &section : change_sections) {
        if (section.type == ChangeSectionType::DELETED) {
            auto size = section.deleted.deleted_area.data() - ptr;
            os.write(ptr, size);
            ptr += size;
            ptr += section.deleted.deleted_area.size();
            continue;
        }
        if (section.type == ChangeSectionType::ADDED) {
            auto size = section.added.insertion_point - ptr;
            if (size < 0) {
                size = 0;
            }
            os.write(ptr, size);
            ptr += size;
            os.write(section.added.new_content, static_cast<std::streamsize>(section.added.new_content_length));
            continue;
        }
        ASSERT(false);
    }

    auto size = sizeInBytes - (ptr - data);
    os.write(ptr, size);

    os.close();

    return true;
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
        return false;
    }

    if (pageNum < 1 || pageNum > count) {
        spdlog::warn("Tried to delete page {}, which is outside of the inclusive range [1, {}]", pageNum, count);
        return false;
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
            for (auto obj : objects) {
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

    return true;
}

void Document::delete_raw_section(std::string_view d) {
    change_sections.push_back({.type = ChangeSectionType::DELETED, .deleted = {.deleted_area = d}});
}

void Document::add_raw_section(char *insertionPoint, char *newContent, size_t newContentLength) {
    change_sections.push_back({.type  = ChangeSectionType::ADDED,
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

} // namespace pdf
