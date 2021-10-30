
#include <spdlog/spdlog.h>

#include "EditorApplication.h"

int main(int argc, char **argv) {
    spdlog::set_level(spdlog::level::trace);
    auto application = EditorApplication::create();
    return application->run(argc, argv);
}
