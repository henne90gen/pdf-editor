#include <pdf/document.h>

struct DeleteArgs {
    int pageNum           = 0;
    DocumentSource source = {};
};

int cmd_delete_page(const DeleteArgs &args) {
    pdf::Document document;
    if (args.source.read_document(document)) {
        return 1;
    }

    if (document.delete_page(args.pageNum)) {
        spdlog::error("Failed to delete page {}", args.pageNum);
        return 1;
    }

    char *buffer;
    size_t size;
    if (document.write_to_memory(buffer, size)) {
        spdlog::error("Failed to save PDF document");
        return 1;
    }

    auto sv = std::string_view(buffer, size);
    std::cout << sv;
    free(buffer);

    return 0;
}
