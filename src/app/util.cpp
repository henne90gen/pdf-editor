#include "util.h"

#include <spdlog/spdlog.h>
#include <sstream>

void help() {
    spdlog::error("Usage:");
    spdlog::error("    cat my-document.pdf | pdf-app [COMMAND]");
    spdlog::error("    pdf-app [COMMAND] [FILE_PATH]");
}

int DocumentSource::read_document(pdf::Document &document) const {
    if (!fromStdin || !filePath.empty()) {
        if (pdf::Document::read_from_file(filePath, document)) {
            spdlog::error("Failed to read document '{}'", filePath);
            return 1;
        }
        return 0;
    }

    std::stringstream ss;
    ss << std::cin.rdbuf();
    auto s = ss.str();
    if (pdf::Document::read_from_memory(s.data(), s.size(), document)) {
        spdlog::error("Failed to read document from stdin");
        return 1;
    }

    return 0;
}
