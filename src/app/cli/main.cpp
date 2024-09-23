#include <fstream>
#include <iostream>
#include <pdf/document.h>

#include "cmd_delete_page.cpp"
#include "cmd_embed.cpp"
#include "cmd_extract.cpp"
#include "cmd_images.cpp"
#include "cmd_info.cpp"
#include "cmd_text.cpp"

void help() {
    spdlog::error("Usage:");
    spdlog::error("    pdf-cli [COMMAND] [FILE_PATH]");
    spdlog::error("");
}

enum class CommandType {
    UNKNOWN,
    INFO,
    DELETE_PAGE,
    CONCATENATE,
    IMAGES,
    EMBED_FILES,
    EXTRACT_FILES,
    TEXT,
};

CommandType parse_command_type(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "info") {
            return CommandType::INFO;
        }
        if (arg == "delete-page") {
            return CommandType::DELETE_PAGE;
        }
        if (arg == "concatenate") {
            return CommandType::CONCATENATE;
        }
        if (arg == "images") {
            return CommandType::IMAGES;
        }
        if (arg == "text") {
            return CommandType::TEXT;
        }
        if (arg == "embed") {
            return CommandType::EMBED_FILES;
        }
        if (arg == "extract") {
            return CommandType::EXTRACT_FILES;
        }
    }

    return CommandType::UNKNOWN;
}

int parse_document_source(int argc, char **argv, int firstArg, std::string_view &documentSource) {
    if (firstArg >= argc) {
        spdlog::error("No PDF document source specified");
        help();
        return 1;
    }

    documentSource = std::string_view(argv[firstArg], strlen(argv[firstArg]));
    return 0;
}

int parse_info_args(int argc, char **argv, InfoArgs &result) {
    return parse_document_source(argc, argv, 2, result.source);
}

int parse_delete_args(int argc, char **argv, DeleteArgs &result) {
    if (argc < 3) {
        return 1;
    }

    if (parse_document_source(argc, argv, 3, result.source)) {
        return 1;
    }

    std::string s = argv[2];
    // TODO catch exceptions
    result.pageNum = std::stoi(s);

    return 0;
}

int parse_images_args(int argc, char **argv, ImagesArgs &result) {
    return parse_document_source(argc, argv, 2, result.source);
}

int parse_embed_args(int argc, char **argv, EmbedArgs &result) {
    int startSource = 2;
    while (startSource < argc) {
        result.files.emplace_back(argv[startSource], strlen(argv[startSource]));
        startSource++;
    }
    return 0;
}

int parse_extract_args(int argc, char **argv, ExtractArgs &result) {
    return parse_document_source(argc, argv, 2, result.source);
}

int parse_text_args(int argc, char **argv, TextArgs &result) {
    return parse_document_source(argc, argv, 2, result.source);
}

int main(int argc, char **argv) {
#ifndef NDEBUG
    spdlog::set_level(spdlog::level::debug);
#endif

    auto commandType = parse_command_type(argc, argv);
    switch (commandType) {
    case CommandType::UNKNOWN:
        help();
        return 1;
    case CommandType::INFO: {
        InfoArgs args;
        if (parse_info_args(argc, argv, args)) {
            return 1;
        }
        return cmd_info(args);
    }
    case CommandType::DELETE_PAGE: {
        DeleteArgs args;
        if (parse_delete_args(argc, argv, args)) {
            return 1;
        }
        return cmd_delete_page(args);
    }
    case CommandType::CONCATENATE:
        // TODO implement concatenation of PDF documents
        return 1;
    case CommandType::IMAGES: {
        ImagesArgs args;
        if (parse_images_args(argc, argv, args)) {
            return 1;
        }
        return cmd_images(args);
    }
    case CommandType::EMBED_FILES: {
        EmbedArgs args;
        if (parse_embed_args(argc, argv, args)) {
            return 1;
        }
        return cmd_embed(args);
    }
    case CommandType::EXTRACT_FILES: {
        ExtractArgs args;
        if (parse_extract_args(argc, argv, args)) {
            return 1;
        }
        return cmd_extract(args);
    }
    case CommandType::TEXT: {
        TextArgs args;
        if (parse_text_args(argc, argv, args)) {
            return 1;
        }
        return cmd_text(args);
    }
    }
}
