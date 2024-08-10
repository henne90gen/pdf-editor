#include <spdlog/spdlog.h>

#include "EditorApplication.h"

int main(int argc, char **argv) {
#ifndef NDEBUG
    spdlog::set_level(spdlog::level::trace);
#endif

    auto application = EditorApplication::create();
    return application->run(argc, argv);
}
