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

struct File : public ReferenceResolver {
    char *data                              = nullptr;
    int64_t sizeInBytes                     = 0;
    Trailer trailer                         = {};
    CrossReferenceTable crossReferenceTable = {};
    std::vector<Object *> objects           = {};

    Object *getObject(int64_t objectNumber);
    Dictionary *getRoot();
    std::vector<Object *> getAllObjects();
    Object *resolve(IndirectReference *ref) override;

  private:
    [[nodiscard]] Object *loadObject(int64_t objectNumber) const;
};

} // namespace pdf
