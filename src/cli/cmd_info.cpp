#include <pdf/document.h>

int cmd_info(std::string &s) {
    pdf::Document document;
    if (pdf::Document::read_from_memory(s.data(), s.size(), document)) {
        spdlog::error("Failed to load PDF document");
        return 1;
    }

    spdlog::set_pattern("%v");

    spdlog::info("Size in bytes:    {:>5}", document.sizeInBytes);
    spdlog::info("Pages:            {:>5}", document.page_count());
//    spdlog::ingo("Lines:         {:>5}", document.line_count());
//    spdlog::info("Words:         {:>5}", document.word_count());
//    spdlog::info("Characters:    {:>5}", document.character_count());
    spdlog::info("Objects:          {:>5} ({} parsable)", document.object_count(), document.objects().size());
    return 0;
}
