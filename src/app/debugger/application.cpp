#include "application.h"

#include <imgui.h>
#include <pdf/document.h>
#include <spdlog/spdlog.h>

struct ApplicationState {
    pdf::Document document;
};

static ApplicationState state;

util::Result application_init(int argc, char **argv) {
    spdlog::info("Arguments:");
    for (int i = 0; i < argc; i++) {
        spdlog::info("  {}", argv[i]);
    }

    if (argc == 1) {
        return util::Result::ok();
    }
    if (argc != 2) {
        return util::Result::error("Wrong number of arguments");
    }

    const std::string filePath = std::string(argv[1]);
    auto result                = pdf::Document::read_from_file(filePath, state.document, false);
    if (result.has_error()) {
        return result;
    }

    spdlog::info("Loaded '{}'", filePath);
    return util::Result::ok();
}

util::Result application_run() {
    ImGui::Begin("Info");
    ImGui::End();

    return util::Result::ok();
}
