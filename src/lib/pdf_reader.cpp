#include "pdf_reader.h"

#include <fstream>
#include <iostream>
#include <unordered_map>
#include <vector>

#include "pdf_file.h"
#include "pdf_lexer.h"
#include "pdf_parser.h"
#include "util.h"

namespace pdf {

Dictionary *parseDict(char *start, size_t length) {
    ASSERT(start != nullptr);
    ASSERT(length > 0);
    // TODO find a better way of passing the data to the parser
    auto text   = StringTextProvider(std::string(start, length));
    auto lexer  = Lexer(text);
    auto parser = Parser(lexer);
    auto result = parser.parse();
    return result->as<Dictionary>();
}

void readTrailer(File &file) {
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

void readCrossReferenceTable(File &file) {
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

bool load_from_file(const std::string &filePath, File &file) {
    auto is = std::ifstream();
    is.open(filePath, std::ios::in | std::ifstream::ate | std::ios::binary);

    if (!is.is_open()) {
        std::cerr << "Failed to open pdf file." << std::endl;
        return false;
    }

    file.sizeInBytes = is.tellg();
    file.data        = (char *)malloc(file.sizeInBytes);

    is.seekg(0);
    is.read(file.data, file.sizeInBytes);

    readTrailer(file);
    readCrossReferenceTable(file);

    return true;
}

} // namespace pdf
