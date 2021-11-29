#include <pdf/document.h>
#include <pdf/page.h>

struct TextArgs {
    std::string_view source = {};
};

int cmd_text(const TextArgs &args) {
    pdf::Document document;
    if (pdf::Document::read_from_file(std::string(args.source), document)) {
        return 1;
    }

    document.for_each_page([](pdf::Page *page) {
        auto textBlocks = page->text_blocks();
        for (auto &textBlock : textBlocks) {
            spdlog::info(textBlock.text);
        }
        return pdf::ForEachResult::CONTINUE;
    });

    return 0;
}
