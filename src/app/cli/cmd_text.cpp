#include <pdf/document.h>
#include <pdf/page.h>

struct TextArgs {
    std::string_view source = {};
};

int cmd_text(const TextArgs &args) {
    auto allocatorResult = pdf::Allocator::create();
    if (allocatorResult.has_error()) {
        spdlog::error("failed to create allocator: {}", allocatorResult.message());
        return 1;
    }

    auto result = pdf::Document::read_from_file(allocatorResult.value(), std::string(args.source));
    if (result.has_error()) {
        spdlog::error(result.message());
        return 1;
    }

    auto &document = result.value();
    document.for_each_page([](pdf::Page *page) {
        auto textBlocks = page->text_blocks();
        for (auto &textBlock : textBlocks) {
            spdlog::info(textBlock.text);
        }
        return pdf::ForEachResult::CONTINUE;
    });

    return 0;
}
