#include "document.h"

#include <fstream>

namespace pdf {

IndirectObject *Document::loadObject(int64_t objectNumber) const {
    auto &entry = crossReferenceTable.entries[objectNumber];
    if (entry.isFree) {
        return nullptr;
    }

    auto start = data + entry.byteOffset;

    // TODO this is dangerous (it might read past the end of the stream)
    size_t length = 0;
    while (std::string(start + length, 6) != "endobj") {
        length++;
    }
    length += 6;

    auto input  = std::string_view(start, length);
    auto text   = StringTextProvider(input);
    auto lexer  = TextLexer(text);
    auto parser = Parser(lexer, (ReferenceResolver *)this);
    auto result = parser.parse();
    return result->as<IndirectObject>();
}

IndirectObject *Document::getObject(int64_t objectNumber) {
    if (objects[objectNumber] != nullptr) {
        return objects[objectNumber];
    }

    auto object           = loadObject(objectNumber);
    objects[objectNumber] = object;
    return object;
}

Dictionary *Document::root() {
    auto root = trailer.dict->values["Root"];
    return get<Dictionary>(root);
}

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
        if (entry.isFree) {
            continue;
        }
        auto object = getObject(i);
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

int64_t Page::rotate() {
    const std::optional<Integer *> &rot = node->attribute<Integer>(document, "Rotate", true);
    if (!rot.has_value()) {
        return 0;
    }
    return rot.value()->value;
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

void readTrailer(Document &file) {
    char *startOfEofMarker = file.data + (file.sizeInBytes - 6);
    if (std::string(startOfEofMarker, 6) != "%%EOF\n") {
        std::cerr << "Last line did not have '%%EOF'" << std::endl;
        return;
    }

    char *lastCrossRefStartPtr = startOfEofMarker - 2;
    while (*lastCrossRefStartPtr != '\n' && file.data < lastCrossRefStartPtr) {
        lastCrossRefStartPtr--;
    }
    if (file.data == lastCrossRefStartPtr) {
        std::cerr << "ERROR: reached start of file" << std::endl;
        return;
    }
    lastCrossRefStartPtr++;

    try {
        file.trailer.lastCrossRefStart =
              std::stoll(std::string(lastCrossRefStartPtr, startOfEofMarker - 1 - lastCrossRefStartPtr));
    } catch (std::invalid_argument &err) {
        // TODO add logging
    } catch (std::out_of_range &err) {
        // TODO add logging
    }

    char *startxrefPtr = lastCrossRefStartPtr - 10;
    auto startxrefLine = std::string(startxrefPtr, 9);
    if (startxrefLine != "startxref") {
        std::cerr << "Expected startxref" << std::endl;
        return;
    }

    char *startOfTrailerPtr = startxrefPtr;
    while (std::string(startOfTrailerPtr, 7) != "trailer" && file.data < startOfTrailerPtr) {
        startOfTrailerPtr--;
    }
    if (file.data == startOfTrailerPtr) {
        std::cerr << "ERROR: reached start of file" << std::endl;
        return;
    }

    startOfTrailerPtr += 8;
    auto lengthOfTrailerDict = startxrefPtr - 1 - startOfTrailerPtr;
    file.trailer.dict        = parseDict(startOfTrailerPtr, lengthOfTrailerDict);
}

void readCrossReferenceTable(Document &file) {
    char *crossRefPtr = file.data + file.trailer.lastCrossRefStart;
    if (std::string(crossRefPtr, 5) != "xref\n") {
        std::cerr << "Expected xref" << std::endl;
        return;
    }
    crossRefPtr += 5;

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
        CrossReferenceEntry entry = {};
        auto s                    = std::string(beginTable, 20);
        // TODO catch exceptions
        entry.byteOffset       = std::stoll(s.substr(0, 10));
        entry.generationNumber = std::stoll(s.substr(11, 16));
        entry.isFree           = s[17] == 'f';
        file.crossReferenceTable.entries.push_back(entry);

        beginTable += 20;
    }
    file.objects.resize(file.crossReferenceTable.entries.size(), nullptr);
}

bool Document::load_from_file(const std::string &filePath, Document &document) {
    auto is = std::ifstream();
    is.open(filePath, std::ios::in | std::ifstream::ate | std::ios::binary);

    if (!is.is_open()) {
        std::cerr << "Failed to open pdf file." << std::endl;
        return false;
    }

    document.sizeInBytes = is.tellg();
    document.data        = (char *)malloc(document.sizeInBytes);

    is.seekg(0);
    is.read(document.data, document.sizeInBytes);

    readTrailer(document);
    readCrossReferenceTable(document);

    return true;
}

} // namespace pdf
