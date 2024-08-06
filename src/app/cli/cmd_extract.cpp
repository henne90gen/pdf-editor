struct ExtractArgs {
    std::string_view source;
};

int cmd_extract(ExtractArgs &args) {
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

    // TODO get the file name from the file specifications

    document.for_each_embedded_file([](pdf::EmbeddedFile * /*file*/) {
        spdlog::info("Found embedded file");

        // TODO check that the file does not exist already (if so add a "-1" to the file name)

        //        auto content = file->decode(document.allocator);
        //        std::ofstream os(std::string(file->name()), std::ios::binary);
        //        os.write(content.data(), static_cast<std::streamsize>(content.size()));
        //        os.close();

        return pdf::ForEachResult::CONTINUE;
    });

    return 0;
}
