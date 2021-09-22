
#include <glibmm/markup.h>
#include <gtkmm/application.h>
#include <gtkmm/builder.h>

#include "MainWindow.h"

struct GuiArgs {
    DocumentSource source;
};

std::string getGladeFile(const std::string &fileName) { return "../../../src/app/" + fileName; }

int cmd_gui(int argc, char **argv, const GuiArgs &args) {
    spdlog::set_level(spdlog::level::trace);

    auto app = Gtk::Application::create(argc, argv, "de.henne90gen.pdf_editor");

    try {
        const std::string &filename = getGladeFile("pdf-editor.glade");
        auto builder                = Gtk::Builder::create_from_file(filename);
        MainWindow *wnd             = nullptr;
        builder->get_widget_derived("MainWindow", wnd);
        auto r = app->run(*wnd);
        delete wnd;
        return r;
    } catch (Gtk::BuilderError &err) { std::cerr << err.what() << std::endl; } catch (Glib::MarkupError &err) {
        std::cerr << err.what() << std::endl;
    }

    return 1;
}
