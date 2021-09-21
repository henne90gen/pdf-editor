#include <spdlog/spdlog.h>
#include <sstream>

#include "cmd_delete_page.cpp"
#include "cmd_info.cpp"

enum class CommandType {
    UNKNOWN,
    INFO,
    DELETE_PAGE,
    CONCATENATE,
};

CommandType parse_command_type(int argc, char **argv) {
    if (argc == 1) {
        return CommandType::INFO;
    }

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "delete-page") {
            return CommandType::DELETE_PAGE;
        }
        if (arg == "concatenate") {
            return CommandType::CONCATENATE;
        }
    }

    return CommandType::UNKNOWN;
}

int parse_delete_arguments(int argc, char **argv) {
    if (argc < 3) {
        return -1;
    }
    std::string s = argv[2];
    // TODO catch exceptions
    return std::stoi(s);
}

int main(int argc, char **argv) {
    std::stringstream ss;
    ss << std::cin.rdbuf();
    std::string s = ss.str();

    auto commandType = parse_command_type(argc, argv);
    switch (commandType) {
    case CommandType::UNKNOWN:
        return 1;
    case CommandType::INFO:
        return cmd_info(s);
    case CommandType::DELETE_PAGE: {
        int pageNum = parse_delete_arguments(argc, argv);
        return cmd_delete_page(s, pageNum);
    }
    case CommandType::CONCATENATE:
        TODO("Concatenating PDF documents is not implemented yet");
        return 1;
    }
}
