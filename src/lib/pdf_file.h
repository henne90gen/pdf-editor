#pragma once

#include "pdf_lexer.h"
#include "pdf_objects.h"
#include "pdf_parser.h"

namespace pdf {

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

struct File {
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

  private:
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

} // namespace pdf
