#include <iostream>

#include <glibmm/convert.h>
#include <gtkmm/applicationwindow.h>
#include <gtkmm/builder.h>
#include <gtkmm/notebook.h>
#include <gtkmm/textview.h>

class MainWindow : public Gtk::ApplicationWindow {
  public:
    [[maybe_unused]] MainWindow(BaseObjectType *obj, Glib::RefPtr<Gtk::Builder> _builder)
        : Gtk::ApplicationWindow(obj), builder(std::move(_builder)) {

        std::vector<Gtk::TargetEntry> targets = {Gtk::TargetEntry("text/uri-list")};
        this->drag_dest_set(targets, Gtk::DEST_DEFAULT_DROP | Gtk::DEST_DEFAULT_HIGHLIGHT | Gtk::DEST_DEFAULT_MOTION,
                            Gdk::ACTION_COPY | Gdk::ACTION_MOVE);
        this->signal_drag_drop().connect(sigc::mem_fun(this, &MainWindow::custom_on_drag_drop));
        this->signal_drag_data_received().connect(sigc::mem_fun(this, &MainWindow::custom_on_drag_data_received));

        builder->get_widget("ContentNotebook", notebook);
        for (int i = 0; i < notebook->get_n_pages(); i++) {
            notebook->remove_page(i);
        }
    }

    ~MainWindow() override = default;

  protected:
    bool custom_on_drag_drop(const Glib::RefPtr<Gdk::DragContext> &context, int x, int y, guint time) {
        // limits the data that is received to "text/uri-list", prevents on_drag_data_received from being triggered more
        // than once
        drag_get_data(context, "text/uri-list", time);
        return true;
    }

    void custom_on_drag_data_received(const Glib::RefPtr<Gdk::DragContext> &context, int x, int y,
                                      const Gtk::SelectionData &selection_data, guint info, guint time) {
        auto uriList = selection_data.get_uris();
        for (auto &uri : uriList) {
            auto fileName = Glib::filename_from_uri(uri);

            auto buffer = Gtk::TextBuffer::create();
            buffer->set_text(fileName);
            pages.emplace_back(buffer);
            notebook->append_page(pages.back(), fileName);
        }
        notebook->show_all();
        context->drag_finish(true, false, time);
    }

  private:
    Glib::RefPtr<Gtk::Builder> builder;
    Gtk::Notebook *notebook;
    std::list<Gtk::TextView> pages;
};

int main(int argc, char *argv[]) {
    auto app     = Gtk::Application::create(argc, argv, "de.henne90gen.pdf_editor");
    auto builder = Gtk::Builder::create_from_file("../../../src/app/pdf-editor.glade");

    MainWindow *wnd = nullptr;
    builder->get_widget_derived("MainWindow", wnd);
    auto r = app->run(*wnd);
    delete wnd;
    return r;
}
