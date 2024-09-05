#include "DebugApplication.h"

#include <glibmm/markup.h>
#include <gtkmm/builder.h>
#include <spdlog/spdlog.h>

#include "DebugWindow.h"

DebugApplication::DebugApplication() : PdfApplication("de.henne90gen.pdf.debugger") {}

void DebugApplication::open_window(const std::string &filePath) {
    spdlog::info("Opening new window: {}", filePath);
    auto allocatorResult = pdf::Allocator::create();
    if (allocatorResult.has_error()) {
        spdlog::error("Failed to create allocator: {}", allocatorResult.message());
        return;
    }

    auto result = pdf::Document::read_from_file(allocatorResult.value(), filePath, false);
    if (result.has_error()) {
        spdlog::error("{}", result.message());
        return;
    }

    auto &document = result.value();
    try {
        auto builder = Gtk::Builder::create_from_resource("/com/github/henne90gen/pdf-debugger/debugger.ui");
        auto window  = Gtk::Builder::get_widget_derived<DebugWindow>(builder, "DebugWindow", document);
        add_window(*window);
        window->present();
    } catch (Gtk::BuilderError &err) {
        spdlog::error("{}", err.what()); //
    } catch (Glib::MarkupError &err) {
        spdlog::error("{}", err.what()); //
    }
}
