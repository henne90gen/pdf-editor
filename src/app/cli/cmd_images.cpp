#include <pdf/document.h>

struct ImagesArgs {
    std::string_view source = {};
};

int cmd_images(const ImagesArgs &args) {
    auto allocatorResult = pdf::Allocator::create();
    if (allocatorResult.has_error()) {
        spdlog::error("Failed to create allocator: {}", allocatorResult.message());
        return 1;
    }

    auto result = pdf::Document::read_from_file(allocatorResult.value(), std::string(args.source));
    if (result.has_error()) {
        return 1;
    }

    auto &document = result.value();
    int count      = 0;
    document.for_each_image([&count, &document](pdf::Image &img) {
        spdlog::info("Found image: width={}, height={}", img.width, img.height);

        const auto &fileName = std::to_string(count) + ".bmp";
        if (img.write_bmp(document.allocator, fileName).has_error()) {
            spdlog::warn("Failed to write image file '{}'", fileName);
            return pdf::ForEachResult::CONTINUE;
        }

        count++;
        return pdf::ForEachResult::CONTINUE;
    });

    spdlog::info("Extracted {} images", count);

    return 0;
}
