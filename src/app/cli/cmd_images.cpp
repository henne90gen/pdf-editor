#include <pdf/document.h>

struct ImagesArgs {
    std::string_view source = {};
};

int cmd_images(const ImagesArgs &args) {
    pdf::Document document;
    if (pdf::Document::read_from_file(std::string(args.source), document).has_error()) {
        return 1;
    }

    int count = 0;
    document.for_each_image([&count](pdf::Image &img) {
        spdlog::info("Found image: width={}, height={}", img.width, img.height);

        const auto &fileName = std::to_string(count) + ".bmp";
        if (img.write_bmp(fileName).has_error()) {
            spdlog::warn("Failed to write image file '{}'", fileName);
            return pdf::ForEachResult::CONTINUE;
        }

        count++;
        return pdf::ForEachResult::CONTINUE;
    });

    spdlog::info("Extracted {} images", count);

    return 0;
}
