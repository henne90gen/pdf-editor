#include <spdlog/spdlog.h>

#include "DebugApplication.h"

int main(int argc, char **argv) {
#ifndef NDEBUG
    spdlog::set_level(spdlog::level::trace);
#endif

    auto application = DebugApplication::create();
    return application->run(argc, argv);
}
