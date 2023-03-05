#include <pdf/document.h>

struct DeleteArgs {
    std::string_view source = {};
    int pageNum             = 0;
};

int cmd_delete_page(const DeleteArgs &args) {
    auto documentResult = pdf::Document::read_from_file(std::string(args.source));
    if (documentResult.has_error()) {
        spdlog::error("Failed to load PDF document: {}", documentResult.message());
        return 1;
    }

    auto document = documentResult.value();
    auto result   = document.delete_page(args.pageNum);
    if (result.has_error()) {
        spdlog::error("Failed to delete page {}: {}", args.pageNum, result.message());
        return 1;
    }

    uint8_t *buffer = nullptr;
    size_t size     = 0;
    result          = document.write_to_memory(buffer, size);
    if (result.has_error()) {
        spdlog::error("Failed to save PDF document: {}", result.message());
        return 1;
    }

    auto sv = std::string_view((char *)buffer, size);
    std::cout << sv;
    free(buffer);

    return 0;
}
