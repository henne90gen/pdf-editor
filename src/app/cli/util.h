#pragma once

#include <pdf/document.h>

void help();

struct DocumentSource {
    bool fromStdin = false;
    std::string filePath;

    int read_document(pdf::Document &document) const;
};
