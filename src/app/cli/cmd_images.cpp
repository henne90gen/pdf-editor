#include <pdf/document.h>

struct ImagesArgs {
    std::string_view source = {};
};

int cmd_images(const ImagesArgs &args) {
    pdf::Document document;
    if (pdf::Document::read_from_file(std::string(args.source), document)) {
        return 1;
    }

    int count = 0;
    document.for_each_image([&count](pdf::Image &img) {
        spdlog::info("Found image: width={}, height={}", img.width, img.height);
        spdlog::info("Dict: {}", pdf::to_string(img.stream->dictionary));
        const auto &fileName = std::to_string(count) + ".bmp";
        if (img.write_bmp(fileName)) {
            spdlog::warn("Failed to write image file '{}'", fileName);
        }
        count++;
        return pdf::ForEachResult::CONTINUE;
    });

    return 0;
}
