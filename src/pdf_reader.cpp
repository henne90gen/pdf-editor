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

Dictionary *parseDict(std::vector<Line> lines, int start, int end) {
    ASSERT(start < end);
    // TODO find a better way of passing the data to the parser
    auto textPtr  = lines[start].ptr;
    auto textSize = static_cast<int64_t>((lines[end].ptr + lines[end].length) - textPtr);
    auto text     = StringTextProvider(std::string(textPtr, textSize));
    auto lexer    = Lexer(text);
    auto parser   = Parser(lexer);
    auto result   = parser.parse();
    return result->as<Dictionary>();
}

IndirectObject *parseIndirectObject(const std::string &input) {
    auto text     = StringTextProvider(input);
    auto lexer    = Lexer(text);
    auto parser   = Parser(lexer);
    auto result   = parser.parse();
    return result->as<IndirectObject>();
}

void readTrailer(const std::vector<Line> &lines, Trailer &trailer) {
    try {
        trailer.lastCrossRefStart = std::stoll(lines[lines.size() - 2].to_string());
    } catch (std::invalid_argument &err) {
        // TODO add logging
    } catch (std::out_of_range &err) {
        // TODO add logging
    }

    int startOfTrailer = -1;
    for (int i = lines.size() - 1; i >= 0; i--) {
        if (lines[i].to_string() == "trailer") {
            startOfTrailer = i + 1;
            break;
        }
    }

    if (startOfTrailer == -1) {
        std::cerr << "Failed to find ptr of trailer" << std::endl;
        return;
    }

    int endOfTrailer = lines.size() - 4;
    trailer.dict     = parseDict(lines, startOfTrailer, endOfTrailer)->as<Dictionary>();
}

void readCrossReferenceTable(char *buf, const Trailer &trailer, CrossReferenceTable &table) {
    char *crossRefPtr = buf + trailer.lastCrossRefStart;
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
    table.firstObjectId = std::stoll(metaData.substr(0, spaceLocation));
    table.objectCount   = std::stoll(metaData.substr(spaceLocation));
    for (int i = 0; i < table.objectCount; i++) {
        // nnnnnnnnnn ggggg f__
        CrossReferenceEntry entry = {};
        auto s                    = std::string(beginTable, 20);
        // TODO catch exceptions
        entry.byteOffset       = std::stoll(s.substr(0, 10));
        entry.generationNumber = std::stoll(s.substr(11, 16));
        entry.isFree           = s[17] == 'f';
        table.entries.push_back(entry);
        beginTable += 20;
    }
}

void pdf_reader::read(const std::string &filePath) {
    auto is = std::ifstream();
    is.open(filePath, std::ios::in | std::ifstream::ate | std::ios::binary);

    if (!is.is_open()) {
        std::cerr << "Failed to open pdf file." << std::endl;
        return;
    }

    auto fileSize = is.tellg();
    char *buf     = (char *)malloc(fileSize);

    is.seekg(0);
    is.read(buf, fileSize);

    std::vector<Line> lines = {};
    char *lastLineStart     = buf;
    for (int i = 0; i < fileSize; i++) {
        if (buf[i] == '\n') {
            unsigned long lineLength = i - (lastLineStart - buf);
            lines.emplace_back(lastLineStart, lineLength);
            lastLineStart = buf + i + 1;
        }
    }

    if (lines.back().to_string() != "%%EOF") {
        std::cerr << "Last line did not have '%%EOF'" << std::endl;
        return;
    }

    auto startxrefLine = lines[lines.size() - 3];
    if (startxrefLine.to_string() != "startxref") {
        std::cerr << "Expected startxref" << std::endl;
        return;
    }

    Trailer trailer = {};
    readTrailer(lines, trailer);

    CrossReferenceTable table = {};
    readCrossReferenceTable(buf, trailer, table);

    std::vector<Object *> objects = {};
    for (auto &entry : table.entries) {
        auto start = buf + entry.byteOffset;

        // TODO this is dangerous (it might read past the end of the stream)
        size_t length = 0;
        while (std::string(start + length, 6) != "endobj") {
            length++;
        }
        auto obj = parseIndirectObject(std::string(start, length + 6));
    }

    for (auto line : lines) {
        std::cout << line.to_string() << std::endl;
    }

    std::cout << std::endl << "Success" << std::endl;
}
