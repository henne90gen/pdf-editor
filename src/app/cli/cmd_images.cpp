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
        img.write_bmp(std::to_string(count) + ".bmp");
        count++;
        return true;
    });

    return 0;
}
