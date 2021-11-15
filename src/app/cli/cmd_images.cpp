#include <pdf/document.h>

struct ImagesArgs {
    DocumentSource source = {};
};

int cmd_images(const ImagesArgs &args) {
    pdf::Document document;
    if (args.source.read_document(document)) {
        return 1;
    }

    int count = 0;
    document.for_each_image([&count](pdf::Image &img) {
        spdlog::info("Found image: width={}, height={}", img.width, img.height);
        spdlog::info("Dict: {}", pdf::to_string(img.stream->dictionary));
        const auto &fileName = std::to_string(count) + ".bmp";
        if (img.write_bmp( fileName)) {
            spdlog::warn("Failed to write image file '{}'", fileName);
        }
        count++;
        return true;
    });

    return 0;
}
