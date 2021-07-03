#include "PdfPage.h"

PdfPage::PdfPage(BaseObjectType *obj, Glib::RefPtr<Gtk::Builder> _builder, std::string _fileName)
    : Gtk::Box(obj), builder(std::move(_builder)), fileName(std::move(_fileName)) {
    std::cout << "Created page for file: " << fileName << std::endl;

    if (!pdf::load_from_file(fileName, file)) {
        return;
    }

    auto root = file.getRoot();
    for (auto &entry : root->values) {
        std::cout << entry.second->type << ": " << entry.first << std::endl;
    }
    std::cout << std::endl;

    auto pages = file.getPages();
    for (auto &entry : pages->values) {
        std::cout << entry.second->type << ": " << entry.first << std::endl;
    }
    std::cout << std::endl;
}
