#include "application.h"

#include <imgui.h>
#include <pdf/document.h>
#include <spdlog/spdlog.h>

struct ApplicationState {
    pdf::Document document;
};

static ApplicationState state;

void application_init(int argc, char **argv) {
    spdlog::info("Arguments:");
    for (int i = 0; i < argc; i++) {
        spdlog::info("  {}", argv[i]);
    }

    if (argc == 1) {
        return;
    }
    if (argc != 2) {
        spdlog::warn("Wrong number of arguments");
        exit(1);
    }

    const std::string filePath = std::string(argv[1]);
    auto result = pdf::Document::read_from_file(filePath, state.document, false);
    if (result.has_error()) {
        spdlog::error(result.message());
        exit(1);
    }
    spdlog::info("Loaded '{}'", filePath);
}

void application_run() {
    ImGui::Begin("Test");
    ImGui::End();
}
