#include <spdlog/spdlog.h>

#include "DebugApplication.h"

int main(int argc, char **argv) {
#ifndef NDEBUG
    spdlog::set_level(spdlog::level::trace);
#endif

    auto application = DebugApplication::create();
    return application->run(argc, argv);
}

//#if _WIN32
//#include <windows.h>
//int WinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, LPSTR /*lpCmdLine*/, int /*nShowCmd*/) {
//    spdlog::info("Using win main");
//    return main(__argc, __argv);
//}
//#endif
