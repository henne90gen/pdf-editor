#include "pdf_reader.h"

#include <fstream>
#include <iostream>
#include <unordered_map>
#include <vector>

#include "Lexer.h"
#include "Parser.h"
#include "util.h"

struct Line {
    char *ptr      = nullptr;
    int32_t length = 0;
    Line()         = default;
    Line(char *_start, int32_t _length) : ptr(_start), length(_length) {}
    [[nodiscard]] std::string to_string() const { return std::string(ptr, length); }
};

struct Trailer {
    int64_t lastCrossRefStart = {};
    Dictionary *dict          = nullptr;
};

struct CrossReferenceEntry {
    int64_t byteOffset       = 0;
    int64_t generationNumber = 0;
    bool isFree              = false;
};

struct CrossReferenceTable {
    int64_t firstObjectId                    = 0;
    int64_t objectCount                      = 0;
    std::vector<CrossReferenceEntry> entries = {};
};

struct PDFFile {
    char *data                              = nullptr;
    int64_t sizeInBytes                     = 0;
    Trailer trailer                         = {};
    CrossReferenceTable crossReferenceTable = {};
    std::vector<Object *> objects           = {};

    Object *getObject(int64_t objectNumber) {
        if (objects[objectNumber] != nullptr) {
            return objects[objectNumber];
        }

        auto object           = loadObject(objectNumber);
        objects[objectNumber] = object;
        return object;
    }

    [[nodiscard]] Object *loadObject(int64_t objectNumber) const {
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

        auto input  = std::string(start, length + 6);
        auto text   = StringTextProvider(input);
        auto lexer  = Lexer(text);
        auto parser = Parser(lexer);
        auto result = parser.parse();
        return result;
    }
};

Dictionary *parseDict(char *start, size_t length) {
    ASSERT(start < end);
    // TODO find a better way of passing the data to the parser
    auto text   = StringTextProvider(std::string(start, length));
    auto lexer  = Lexer(text);
    auto parser = Parser(lexer);
    auto result = parser.parse();
    return result->as<Dictionary>();
}

void readTrailer(PDFFile &file) {
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

    startOfTrailerPtr += 7;
    auto lengthOfTrailerDict = startxrefPtr - 1 - startOfTrailerPtr;
    file.trailer.dict        = parseDict(startOfTrailerPtr, lengthOfTrailerDict);
}

void readCrossReferenceTable(PDFFile &file) {
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

void pdf_reader::read(const std::string &filePath) {
    auto is = std::ifstream();
    is.open(filePath, std::ios::in | std::ifstream::ate | std::ios::binary);

    if (!is.is_open()) {
        std::cerr << "Failed to open pdf file." << std::endl;
        return;
    }

    PDFFile file     = {};
    file.sizeInBytes = is.tellg();
    file.data        = (char *)malloc(file.sizeInBytes);

    is.seekg(0);
    is.read(file.data, file.sizeInBytes);

    readTrailer(file);
    readCrossReferenceTable(file);

    std::cout << std::string(file.data, file.sizeInBytes) << std::endl;

    std::cout << std::endl << "Success" << std::endl;
}
