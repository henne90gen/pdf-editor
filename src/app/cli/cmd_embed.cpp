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

    pdf::Document document;
    if (pdf::Document::read_from_file(std::string(args.files[args.files.size() - 1]), document).has_error()) {
        return 1;
    }

    for (size_t i = 0; i < args.files.size() - 1; i++) {
        document.embed_file(std::string(args.files[i]));
    }

    if (document.write_to_file("out.pdf").has_error()) {
        return 1;
    }
    return 0;
}
