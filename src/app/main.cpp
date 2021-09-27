#include <iostream>
#include <pdf/document.h>
#include <spdlog/spdlog.h>
#include <sstream>
#include <unistd.h>

void help() {
    spdlog::error("Usage:");
    spdlog::error("    cat my-document.pdf | pdf-app [COMMAND]");
    spdlog::error("    pdf-app [COMMAND] [FILE_PATH]");
}

struct DocumentSource {
    bool fromStdin = false;
    std::string filePath;

    int read_document(pdf::Document &document) const {
        if (!fromStdin || !filePath.empty()) {
            if (pdf::Document::read_from_file(filePath, document)) {
                spdlog::error("Failed to read document '{}'", filePath);
                return 1;
            }
            return 0;
        }

        std::stringstream ss;
        ss << std::cin.rdbuf();
        auto s = ss.str();
        if (pdf::Document::read_from_memory(s.data(), s.size(), document)) {
            spdlog::error("Failed to read document from stdin");
            return 1;
        }

        return 0;
    }
};

#include "cmd_delete_page.cpp"
#include "cmd_gui.cpp"
#include "cmd_info.cpp"

enum class CommandType {
    UNKNOWN,
    INFO,
    DELETE_PAGE,
    CONCATENATE,
    GUI,
};

CommandType parse_command_type(int argc, char **argv) {
    if (argc == 1) {
        return CommandType::GUI;
    }

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
    }

    return CommandType::UNKNOWN;
}

int parse_document_source(int argc, char **argv, int firstArg, DocumentSource &documentSource) {
    bool isTerminal = isatty(fileno(stdin));
    if (isTerminal && firstArg >= argc) {
        spdlog::error("No PDF document source specified");
        help();
        return 1;
    }

    documentSource = {.fromStdin = !isTerminal, .filePath = std::string(argv[firstArg])};
    return 0;
}

int parse_info_arguments(int argc, char **argv, InfoArgs &result) {
    return parse_document_source(argc, argv, 2, result.source);
}

int parse_delete_arguments(int argc, char **argv, DeleteArgs &result) {
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

int parse_gui_args(int argc, char **argv, GuiArgs &result) {
    return parse_document_source(argc, argv, 2, result.source);
}

// pdf my.pdf                     -> opens gui
// cat my.pdf | pdf               -> opens gui
// pdf delete-page 1 my.pdf       -> deletes page 1 from my.pdf and dumps the result to stdout
// cat my.pdf | pdf delete-page 1 -> deletes page 1 from my.pdf and dumps the result to stdout
// pdf info my.pdf                -> shows info
// cat my.pdf | pdf info          -> shows info
int main(int argc, char **argv) {
    auto commandType = parse_command_type(argc, argv);
    switch (commandType) {
    case CommandType::UNKNOWN:
        help();
        return 1;
    case CommandType::INFO: {
        InfoArgs args;
        if (parse_info_arguments(argc, argv, args)) {
            return 1;
        }
        return cmd_info(args);
    }
    case CommandType::DELETE_PAGE: {
        DeleteArgs args;
        if (parse_delete_arguments(argc, argv, args)) {
            return 1;
        }
        return cmd_delete_page(args);
    }
    case CommandType::CONCATENATE:
        TODO("Concatenating PDF documents is not implemented yet");
        return 1;
    case CommandType::GUI:
        GuiArgs args;
        if (parse_gui_args(argc, argv, args)) {
            return 1;
        }
        return cmd_gui(argc, argv, args);
    }
}
