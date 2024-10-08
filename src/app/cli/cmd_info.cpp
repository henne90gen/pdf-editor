#include <pdf/document.h>

struct InfoArgs {
    std::string_view source = {};
};

int cmd_info(const InfoArgs &args) {
    auto allocatorResult = pdf::Allocator::create();
    if (allocatorResult.has_error()) {
        spdlog::error("Failed to create allocator: {}", allocatorResult.message());
        return 1;
    }

    auto result = pdf::Document::read_from_file(allocatorResult.value(), std::string(args.source));
    if (result.has_error()) {
        spdlog::error(result.message());
        return 1;
    }

    auto &document             = result.value();
    size_t pageCount           = document.page_count();
    size_t lineCount           = document.line_count();
    size_t wordCount           = document.word_count();
    size_t characterCount      = document.character_count();
    size_t objectCount         = document.object_count(false);
    size_t parsableObjectCount = document.objects().size();

    spdlog::info("Size in bytes: {:>5}", document.file.sizeInBytes);
    spdlog::info("Pages:         {:>5}", pageCount);
    spdlog::info("Lines:         {:>5}", lineCount);
    spdlog::info("Words:         {:>5}", wordCount);
    spdlog::info("Characters:    {:>5}", characterCount);
    spdlog::info("Objects:       {:>5} ({} parsable)", objectCount, parsableObjectCount);

    return 0;
}
