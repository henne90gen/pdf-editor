#include <pdf/document.h>

struct InfoArgs {
    std::string_view source = {};
};

std::string formatSizeInBytes(size_t size) {
    static std::array<std::string, 7> s = {"B", "KiB", "MiB", "GiB", "TiB", "PiB", "EiB"};
    if (size < 1024) {
        return std::format("{} {}", size, s[0]);
    }

    int size_count = 1;
    while (size / (size_count * 1024) > 1024 and size_count < (int)s.size() - 1) {
        size_count++;
    }

    return std::format("{:.2f} {}", (float)size / ((float)size_count * 1024.0), s[size_count]);
}

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

    spdlog::info("Size:       {:>12}", formatSizeInBytes(document.file.sizeInBytes));
    spdlog::info("Pages:      {:>5}", pageCount);
    spdlog::info("Lines:      {:>5}", lineCount);
    spdlog::info("Words:      {:>5}", wordCount);
    spdlog::info("Characters: {:>5}", characterCount);
    spdlog::info("Objects:    {:>5} ({} parsable)", objectCount, parsableObjectCount);

    return 0;
}
