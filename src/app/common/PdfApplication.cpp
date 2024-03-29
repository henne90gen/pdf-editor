#include "PdfApplication.h"

#include <glibmm/convert.h>
#include <glibmm/markup.h>
#include <gtkmm/builder.h>
#include <spdlog/spdlog.h>

#include "OpenWindow.h"

PdfApplication::PdfApplication(const std::string &appId)
    : Gtk::Application(appId, Gio::Application::Flags::HANDLES_OPEN) {}

void PdfApplication::on_activate() {
    spdlog::info("Activating application");

    try {
        auto builder = Gtk::Builder::create_from_resource("/com/github/henne90gen/pdf-common/../common/open.ui");
        auto window =
              Gtk::Builder::get_widget_derived<OpenWindow>(builder, "OpenWindow", [this](const std::string &filePath) {
                  spdlog::info("Opening file '{}'", filePath);
                  open_window(filePath);
              });
        add_window(*window);
        window->present();
    } catch (Gtk::BuilderError &err) {
        spdlog::error("{}", err.what()); //
    } catch (Glib::MarkupError &err) {
        spdlog::error("{}", err.what()); //
    }
}

void PdfApplication::on_open(const Gio::Application::type_vec_files &files, const Glib::ustring & /*hint*/) {
    if (files.empty()) {
        spdlog::warn("There were no files to open");
        return;
    }

    if (files.size() == 1) {
        spdlog::info("Opening 1 file");
    } else {
        spdlog::info("Opening {} files", files.size());
    }

    for (auto &file : files) {
        auto filePath = Glib::locale_from_utf8(file->get_path());
        open_window(filePath);
    }
}
