struct EmbedArgs {
    std::vector<std::string_view> files;
};

int cmd_embed(EmbedArgs &args) {
    if (args.files.size() < 2) {
        spdlog::error("Not enough files specified: {}", args.files.size());
        spdlog::error("Specify at least 2 files like this:");
        spdlog::error("    pdf-cli embed [FILES_TO_EMBED...] [PDF_FILE]");
        return 1;
    }

    auto allocatorResult = pdf::Allocator::create();
    if (allocatorResult.has_error()) {
        spdlog::error("Failed to create allocator: {}", allocatorResult.message());
        return 1;
    }

    auto documentResult =
          pdf::Document::read_from_file(allocatorResult.value(), std::string(args.files[args.files.size() - 1]));
    if (documentResult.has_error()) {
        spdlog::error(documentResult.message());
        return 1;
    }

    auto &document = documentResult.value();
    for (size_t i = 0; i < args.files.size() - 1; i++) {
        document.embed_file(std::string(args.files[i]));
    }

    if (document.write_to_file("out.pdf").has_error()) {
        return 1;
    }

    return 0;
}
