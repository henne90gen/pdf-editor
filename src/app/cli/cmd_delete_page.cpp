#include <pdf/document.h>

struct DeleteArgs {
    std::string_view source = {};
    int pageNum             = 0;
};

int cmd_delete_page(const DeleteArgs &args) {
    pdf::Document document;
    auto result = pdf::Document::read_from_file(std::string(args.source), document);
    if (result.has_error()) {
        spdlog::error("Failed to load PDF document: {}", result.message());
        return 1;
    }

    result = document.delete_page(args.pageNum);
    if (result.has_error()) {
        spdlog::error("Failed to delete page {}: {}", args.pageNum, result.message());
        return 1;
    }

    char *buffer;
    size_t size;
    result = document.write_to_memory(buffer, size);
    if (result.has_error()) {
        spdlog::error("Failed to save PDF document: {}", result.message());
        return 1;
    }

    auto sv = std::string_view(buffer, size);
    std::cout << sv;
    free(buffer);

    return 0;
}
