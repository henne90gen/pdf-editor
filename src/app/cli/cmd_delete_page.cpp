#include <pdf/document.h>

struct DeleteArgs {
    std::string_view source = {};
    int pageNum             = 0;
};

int cmd_delete_page(const DeleteArgs &args) {
    pdf::Document document;
    if (pdf::Document::read_from_file(std::string(args.source), document).has_error()) {
        return 1;
    }

    if (document.delete_page(args.pageNum)) {
        spdlog::error("Failed to delete page {}", args.pageNum);
        return 1;
    }

    char *buffer;
    size_t size;
    if (document.write_to_memory(buffer, size).has_error()) {
        spdlog::error("Failed to save PDF document");
        return 1;
    }

    auto sv = std::string_view(buffer, size);
    std::cout << sv;
    free(buffer);

    return 0;
}
