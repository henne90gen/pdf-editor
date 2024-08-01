#include <pdf/document.h>
#include <pdf/page.h>

struct TextArgs {
    std::string_view source = {};
};

int cmd_text(const TextArgs &args) {
    auto result = pdf::Document::read_from_file(std::string(args.source));
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
