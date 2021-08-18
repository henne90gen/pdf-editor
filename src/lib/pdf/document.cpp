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
        ASSERT(stream->dictionary->values["Type"]->as<Name>()->value == "ObjStm");
        auto content       = stream->to_string();
        auto textProvider  = StringTextProvider(content);
        auto lexer         = TextLexer(textProvider);
        auto parser        = Parser(lexer, this);
        int64_t N          = stream->dictionary->values["N"]->as<Integer>()->value;
        auto objectNumbers = std::vector<int64_t>(N);
        for (int i = 0; i < N; i++) {
            auto objectNumber = parser.parse()->as<Integer>();
            objectNumbers[i]  = objectNumber->value;
            auto byteOffset   = parser.parse();
        }
        auto objects = std::vector<Object *>(N);
        for (int i = 0; i < N; i++) {
            auto obj   = parser.parse();
            objects[i] = obj;
        }
        return new IndirectObject(objectNumbers[entry.compressed.indexInStream], 0,
                                  objects[entry.compressed.indexInStream]);
    }
}

IndirectObject *Document::getObject(uint64_t objectNumber) {
    if (objects[objectNumber] != nullptr) {
        return objects[objectNumber];
    }

    auto object           = loadObject(objectNumber);
    objects[objectNumber] = object;
    return object;
}

Dictionary *Document::root() { return trailer.root(*this); }

std::vector<Page *> Document::pages() {
    auto root         = this->root();
    auto pageTreeRoot = get<PageTreeNode>(root->values["Pages"]);

    if (pageTreeRoot->isPage()) {
        return {new Page(*this, pageTreeRoot)};
    }

    auto result = std::vector<Page *>();

    std::vector<PageTreeNode *> queue = {pageTreeRoot};
    while (!queue.empty()) {
        PageTreeNode *current = queue.back();
        queue.pop_back();

        for (auto kid : current->kids()->values) {
            auto resolvedKid = get<PageTreeNode>(kid);
            if (resolvedKid->isPage()) {
                result.push_back(new Page(*this, resolvedKid));
            } else {
                queue.push_back(resolvedKid);
            }
        }
    }

    return result;
}

IndirectObject *Document::resolve(const IndirectReference *ref) { return getObject(ref->objectNumber); }

std::vector<IndirectObject *> Document::getAllObjects() {
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
                entry.type                                = CrossReferenceEntryType ::FREE;
                entry.free.nextFreeObjectNumber           = field1;
                entry.free.nextFreeObjectGenerationNumber = field2;
                file.crossReferenceTable.entries.push_back(entry);
            } break;
            case 1: {
                CrossReferenceEntry entry     = {};
                entry.type                    = CrossReferenceEntryType ::NORMAL;
                entry.normal.byteOffset       = field1;
                entry.normal.generationNumber = field2;
                file.crossReferenceTable.entries.push_back(entry);
            } break;
            case 2: {
                CrossReferenceEntry entry             = {};
                entry.type                            = CrossReferenceEntryType ::COMPRESSED;
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

bool Document::loadFromFile(const std::string &filePath, Document &document) {
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

bool Document::saveToFile(const std::string &filePath) const {
    auto os = std::ofstream();
    os.open(filePath, std::ios::out | std::ios::binary);

    if (!os.is_open()) {
        spdlog::error("Failed to open pdf file for writing");
        return false;
    }

    os.write(data, sizeInBytes);

    return true;
}

Dictionary *Trailer::root(Document &document) const {
    if (dict != nullptr) {
        return document.get<Dictionary>(dict->values["Root"]);
    } else {
        Object *root = stream->dictionary->values["Root"];
        return document.get<Dictionary>(root);
    }
}

} // namespace pdf
