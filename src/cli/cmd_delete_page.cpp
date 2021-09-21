#include <pdf/document.h>

int cmd_delete_page(std::string &s, int pageNum) {
    pdf::Document document;
    pdf::Document::load_from_memory(s.data(), s.size(), document);
    if (document.delete_page(pageNum)) {
        spdlog::error("Failed to delete page {}", pageNum);
        return 1;
    }

    char *buffer;
    size_t size;
    if (document.save_to_memory(buffer, size)) {
        spdlog::error("Failed to save PDF document");
        return 1;
    }

    auto sv = std::string_view(buffer, size);
    std::cout << sv;
    free(buffer);

    return 0;
}
