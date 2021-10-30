#include <spdlog/spdlog.h>

#include "DebugApplication.h"

int main(int argc, char **argv) {
    spdlog::set_level(spdlog::level::trace);
    auto application = DebugApplication::create();
    return application->run(argc, argv);
}
