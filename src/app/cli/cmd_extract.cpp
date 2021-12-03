struct ExtractArgs {
    std::string_view source;
};

int cmd_extract(ExtractArgs &args) {
    pdf::Document document;
    if (pdf::Document::read_from_file(std::string(args.source), document).has_error()) {
        return 1;
    }

    document.for_each_embedded_file([&document](pdf::EmbeddedFile *file) {
        spdlog::info("Found embedded file: {}", file->name());

        // TODO check that the file does not exist already (if so add a "-1" to the file name)

        auto content = file->decode(document.allocator);
        std::ofstream os(std::string(file->name()), std::ios::binary);
        os.write(content.data(), static_cast<std::streamsize>(content.size()));
        os.close();

        return pdf::ForEachResult::CONTINUE;
    });

    return 0;
}
