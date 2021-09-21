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
    } else {
        auto stream = get_object(entry.compressed.objectNumberOfStream)->object->as<Stream>();
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

IndirectObject *Document::get_object(int64_t objectNumber) {
    if (objects[objectNumber] != nullptr) {
        return objects[objectNumber];
    }

    auto object           = load_object(objectNumber);
    objects[objectNumber] = object;
    return object;
}

IndirectObject *Document::resolve(const IndirectReference *ref) { return get_object(ref->objectNumber); }

std::vector<IndirectObject *> Document::get_all_objects() {
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
        return true;
    }

    char *lastCrossRefStartPtr = eofMarkerStart - 2;
    while (*lastCrossRefStartPtr != '\n' && file.data < lastCrossRefStartPtr) {
        lastCrossRefStartPtr--;
    }
    if (file.data == lastCrossRefStartPtr) {
        spdlog::error("Unexpectedly reached start of file");
        return true;
    }
    lastCrossRefStartPtr++;

    try {
        file.trailer.lastCrossRefStart =
              std::stoll(std::string(lastCrossRefStartPtr, eofMarkerStart - 1 - lastCrossRefStartPtr));
    } catch (std::invalid_argument &err) {
        spdlog::error("Failed to parse byte offset of cross reference table: {}", err.what());
        return true;
    } catch (std::out_of_range &err) {
        spdlog::error("Failed to parse byte offset of cross reference table: {}", err.what());
        return true;
    }

    const auto xrefKeyword = std::string_view(file.data + file.trailer.lastCrossRefStart, 4);
    if (xrefKeyword != "xref") {
        char *startxrefPtr    = lastCrossRefStartPtr - 10;
        auto startOfStream    = file.data + file.trailer.lastCrossRefStart;
        size_t lengthOfStream = startxrefPtr - startOfStream;
        file.trailer.set_stream(parseStream(startOfStream, lengthOfStream));
        return false;
    } else {
        char *startxrefPtr = lastCrossRefStartPtr - 10;
        auto startxrefLine = std::string_view(startxrefPtr, 9);
        if (startxrefLine == "tartxref\r") {
            startxrefPtr -= 1;
            startxrefLine = std::string_view(startxrefPtr, 9);
        }
        if (startxrefLine != "startxref") {
            spdlog::error("Expected 'startxref', but got '{}'", startxrefLine);
            return true;
        }

        char *startOfTrailerPtr = startxrefPtr;
        while (std::string_view(startOfTrailerPtr, 7) != "trailer" && file.data < startOfTrailerPtr) {
            startOfTrailerPtr--;
        }
        if (file.data == startOfTrailerPtr) {
            spdlog::error("Unexpectedly reached start of file");
            return true;
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
        return false;
    }
}

bool readCrossReferenceTable(Document &file) {
    // exactly one of 'stream' or 'dict' has to be non-null
    ASSERT(file.trailer.get_stream() != nullptr || file.trailer.get_dict() != nullptr);
    ASSERT(file.trailer.get_stream() == nullptr || file.trailer.get_dict() == nullptr);

    if (file.trailer.get_dict() != nullptr) {
        char *crossRefPtr = file.data + file.trailer.lastCrossRefStart;
        if (std::string(crossRefPtr, 4) != "xref") {
            spdlog::error("Expected keyword 'xref' at byte {}", file.trailer.lastCrossRefStart);
            return true;
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
        file.crossReferenceTable.firstObjectNumber = std::stoll(metaData.substr(0, spaceLocation));
        file.crossReferenceTable.objectCount       = std::stoll(metaData.substr(spaceLocation));
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
    return false;
}

bool Document::load_from_file(const std::string &filePath, Document &document) {
    auto is = std::ifstream();
    is.open(filePath, std::ios::in | std::ifstream::ate | std::ios::binary);

    if (!is.is_open()) {
        spdlog::error("Failed to open pdf file for reading");
        return true;
    }

    document.sizeInBytes = is.tellg();
    document.data        = (char *)malloc(document.sizeInBytes);

    is.seekg(0);
    is.read(document.data, static_cast<std::streamsize>(document.sizeInBytes));
    is.close();

    if (readTrailer(document)) {
        return true;
    }
    if (readCrossReferenceTable(document)) {
        return true;
    }

    return false;
}

bool Document::write_to_stream(std::ostream &s) {
    if (changeSections.empty()) {
        s.write(data, static_cast<std::streamsize>(sizeInBytes));
        return s.bad();
    }

    size_t bytesWrittenUntilXref = 0;

    std::sort(changeSections.begin(), changeSections.end(), [](const ChangeSection &a, const ChangeSection &b) {
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
    write_content(s, ptr, bytesWrittenUntilXref);
    if (s.bad()) {
        return true;
    }

    // NOTE write everything up until the cross-reference table
    ASSERT(ptr <= data + trailer.lastCrossRefStart);
    auto size = trailer.lastCrossRefStart - (ptr - data);
    s.write(ptr, static_cast<std::streamsize>(size));
    bytesWrittenUntilXref += size;

    write_new_cross_ref_table(s);
    write_trailer_dict(s, bytesWrittenUntilXref);
    return s.bad();
}

void Document::write_content(std::ostream &s, char *&ptr, size_t &bytesWrittenUntilXref) {
    for (const auto &section : changeSections) {
        if (section.type == ChangeSectionType::DELETED) {
            auto size = section.deleted.deleted_area.data() - ptr;
            s.write(ptr, size);
            if (s.bad()) {
                return;
            }
            bytesWrittenUntilXref += size;

            ptr += size;
            ptr += section.deleted.deleted_area.size();
            continue;
        }
        if (section.type == ChangeSectionType::ADDED) {
            auto size = section.added.insertion_point - ptr;
            if (size < 0) {
                size = 0;
            }

            s.write(ptr, size);
            if (s.bad()) {
                return;
            }
            bytesWrittenUntilXref += size;

            ptr += size;
            s.write(section.added.new_content, static_cast<std::streamsize>(section.added.new_content_length));
            if (s.bad()) {
                return;
            }
            bytesWrittenUntilXref += section.added.new_content_length;
            continue;
        }
        ASSERT(false);
    }
}

void write_zero_padded_number(std::ostream &s, uint64_t num, int64_t maxNumDigits) {
    int64_t numDigits = 0;
    if (num != 0) {
        numDigits = static_cast<int64_t>(std::log10(num) + 1);
    }
    ASSERT(numDigits <= maxNumDigits);

    for (int i = 0; i < maxNumDigits - numDigits; i++) {
        s.write("0", 1);
    }

    if (num != 0) {
        auto tmp = std::to_string(num);
        s.write(tmp.c_str(), static_cast<std::streamsize>(tmp.size()));
    }
}

void Document::write_new_cross_ref_table(std::ostream &s) {
    struct TempXRefEntry {
        int64_t objectNumber      = 0;
        CrossReferenceEntry entry = {};
    };

    auto crossReferenceEntries = std::vector<TempXRefEntry>(crossReferenceTable.entries.size());
    for (int i = 0; i < crossReferenceTable.entries.size(); i++) {
        crossReferenceEntries[i] = {.objectNumber = i, .entry = crossReferenceTable.entries[i]};
    }

    std::sort(crossReferenceEntries.begin(), crossReferenceEntries.end(),
              [](const TempXRefEntry &a, const TempXRefEntry &b) {
                  if (a.entry.type == CrossReferenceEntryType::FREE ||
                      b.entry.type == CrossReferenceEntryType::COMPRESSED) {
                      return true;
                  }
                  if (b.entry.type == CrossReferenceEntryType::FREE ||
                      a.entry.type == CrossReferenceEntryType::COMPRESSED) {
                      return false;
                  }
                  ASSERT(a.entry.type == CrossReferenceEntryType::NORMAL &&
                         b.entry.type == CrossReferenceEntryType::NORMAL);
                  return a.entry.normal.byteOffset < b.entry.normal.byteOffset;
              });

    /*
     *      ↓       ↓
     * |---------------------|
     *        ↑___
     */

    int changeSectionIndex = 0;
    int64_t offset         = 0;
    for (auto &crossRefEntry : crossReferenceEntries) {
        if (crossRefEntry.entry.type == CrossReferenceEntryType::COMPRESSED) {
            TODO("Implement support for rewriting compressed cross reference entries");
            continue;
        }

        if (crossRefEntry.entry.type == CrossReferenceEntryType::FREE) {
            // TODO is there something that needs to be done here?
            continue;
        }

        ASSERT(crossRefEntry.entry.type == CrossReferenceEntryType::NORMAL);

        while (changeSectionIndex < changeSections.size()) {
            auto &changeSection = changeSections[changeSectionIndex];
            if (changeSection.type == ChangeSectionType::DELETED) {
                size_t deletedOffset = changeSection.deleted.deleted_area.data() - data;
                if (deletedOffset > crossRefEntry.entry.normal.byteOffset) {
                    crossRefEntry.entry.normal.byteOffset += offset;
                    break;
                }

                offset -= static_cast<int64_t>(changeSection.deleted.deleted_area.size());
                changeSectionIndex++;
                continue;
            }
            if (changeSection.type == ChangeSectionType::ADDED) {
                size_t addedOffset = changeSection.added.insertion_point - data;
                if (addedOffset > crossRefEntry.entry.normal.byteOffset) {
                    crossRefEntry.entry.normal.byteOffset += offset;
                    break;
                }

                offset += static_cast<int64_t>(changeSection.added.new_content_length);
                changeSectionIndex++;
                continue;
            }
            break;
        }

        if (changeSectionIndex >= changeSections.size()) {
            crossRefEntry.entry.normal.byteOffset += offset;
            continue;
        }
    }

    auto xrefKeyword = "xref\n";
    s.write(xrefKeyword, 5);
    auto firstObjectNumberStr = std::to_string(crossReferenceTable.firstObjectNumber);
    s.write(firstObjectNumberStr.c_str(), static_cast<std::streamsize>(firstObjectNumberStr.size()));
    s.write(" ", 1);
    auto objectCountStr = std::to_string(crossReferenceTable.objectCount);
    s.write(objectCountStr.c_str(), static_cast<std::streamsize>(objectCountStr.size()));
    s.write("\n", 1);

    std::sort(crossReferenceEntries.begin(), crossReferenceEntries.end(),
              [](const TempXRefEntry &a, const TempXRefEntry &b) { return a.objectNumber < b.objectNumber; });
    for (auto &entry : crossReferenceEntries) {
        // TODO iterate over all entries and write them to the output stream
        if (entry.entry.type == CrossReferenceEntryType::NORMAL) {
            write_zero_padded_number(s, entry.entry.normal.byteOffset, 10);
            s.write(" ", 1);
            write_zero_padded_number(s, entry.entry.normal.generationNumber, 5);
            s.write(" n \n", 4);
            continue;
        }
        if (entry.entry.type == CrossReferenceEntryType::FREE) {
            write_zero_padded_number(s, entry.entry.free.nextFreeObjectNumber, 10);
            s.write(" ", 1);
            write_zero_padded_number(s, entry.entry.free.nextFreeObjectGenerationNumber, 5);
            s.write(" f \n", 4);
            continue;
        }
        ASSERT(false);
    }
}

void Document::write_trailer_dict(std::ostream &s, size_t bytesWrittenUntilXref) {
    s.write("trailer\n", 8);
    s.write(trailer.get_dict()->data.data(), static_cast<std::streamsize>(trailer.get_dict()->data.size()));
    s.write("\nstartxref\n", 11);
    auto tmp = std::to_string(bytesWrittenUntilXref);
    s.write(tmp.c_str(), static_cast<std::streamsize>(tmp.size()));
    s.write("\n%%EOF", 6);
}

bool Document::save_to_file(const std::string &filePath) {
    auto os = std::ofstream();
    os.open(filePath, std::ios::out | std::ios::binary);

    if (!os.is_open()) {
        spdlog::error("Failed to open pdf file for writing");
        return true;
    }

    auto result = write_to_stream(os);
    os.close();
    return result;
}

bool Document::save_to_memory(char *&buffer, size_t &size) {
    std::stringstream ss;

    auto error = write_to_stream(ss);
    if (error) {
        return true;
    }

    auto result = ss.str();
    size        = result.size();
    buffer      = (char *)malloc(size);
    memcpy(buffer, result.data(), size);
    return false;
}

bool Document::load_from_memory(char *buffer, size_t size, Document &document) {
    document.data        = buffer;
    document.sizeInBytes = size;

    if (readTrailer(document)) {
        return true;
    }
    if (readCrossReferenceTable(document)) {
        return true;
    }

    return false;
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

} // namespace pdf
