#include <spdlog/spdlog.h>

#include "editor_application.h"

int main(int argc, char **argv) {
#ifndef NDEBUG
    spdlog::set_level(spdlog::level::info);
#endif

    auto application = EditorApplication::create();
    return application->run(argc, argv);
}
