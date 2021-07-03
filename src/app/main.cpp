#include <filesystem>
#include <iostream>

#include <glibmm/convert.h>
#include <glibmm/markup.h>
#include <gtkmm/applicationwindow.h>
#include <gtkmm/box.h>
#include <gtkmm/builder.h>
#include <gtkmm/notebook.h>
#include <gtkmm/textview.h>

#include <pdf_file.h>
#include <pdf_reader.h>

std::string getGladeFile(const std::string &fileName) { return "../../../src/app/" + fileName; }

class PdfPage : public Gtk::Box {
  public:
    PdfPage(BaseObjectType *obj, Glib::RefPtr<Gtk::Builder> _builder, std::string _fileName)
        : Gtk::Box(obj), builder(std::move(_builder)), fileName(std::move(_fileName)) {
        std::cout << "Created page for file: " << fileName << std::endl;

        if (!pdf::load_from_file(fileName, file)) {
            return;
        }

        for (int i = 0; i < file.crossReferenceTable.entries.size(); i++) {
            auto &entry = file.crossReferenceTable.entries[i];
            if (entry.isFree) {
                continue;
            }
            auto obj = file.getObject(i);

        }
    }

  private:
    Glib::RefPtr<Gtk::Builder> builder;
    std::string fileName;
    pdf::File file;
};

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

        add_pdf_page("../../../test-files/blank.pdf");
        notebook->show_all();
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
            add_pdf_page(fileName);
        }
        notebook->show_all();
        context->drag_finish(true, false, time);
    }

    void add_pdf_page(const std::string &filePath) {
        const std::string &gladeTemplate = getGladeFile("pdf-page.glade");
        auto b                           = Gtk::Builder::create_from_file(gladeTemplate);
        PdfPage *page                    = nullptr;
        b->get_widget_derived("PdfPage", page, filePath);
        page->unparent();

        pages.emplace_back(page);

        auto lastSlash = filePath.find_last_of('/');
        auto fileName  = filePath.substr(lastSlash + 1);
        notebook->append_page(*pages.back(), fileName);
    }

  private:
    Glib::RefPtr<Gtk::Builder> builder;
    Gtk::Notebook *notebook = nullptr;

    // using std::list here, because the stored objects have to have stable addresses
    std::list<PdfPage *> pages = {};
};

int main(int argc, char *argv[]) {
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
