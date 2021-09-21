#include <pdf/document.h>

int cmd_info(std::string &s) {
    pdf::Document document;
    if (pdf::Document::load_from_memory(s.data(), s.size(), document)) {
        spdlog::error("Failed to load PDF document");
        return 1;
    }

    spdlog::info("Number of pages: {}", document.page_count());
    return 0;
}
