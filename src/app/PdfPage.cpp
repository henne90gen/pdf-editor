#include "PdfPage.h"

PdfPage::PdfPage(BaseObjectType *obj, Glib::RefPtr<Gtk::Builder> _builder, std::string _fileName)
    : Gtk::Box(obj), builder(std::move(_builder)), fileName(std::move(_fileName)) {
    std::cout << "Created page for file: " << fileName << std::endl;

    if (!pdf::load_from_file(fileName, file)) {
        return;
    }

    auto root = file.getRoot();
    if (root == nullptr) {
        return;
    }
    for (auto &entry : root->values) {
        std::cout << entry.first << ": " << entry.second->type << " | " << entry.second << std::endl;
    }
    std::cout << std::endl;

    auto pages = file.getPages();
    if (pages == nullptr) {
        return;
    }
    for (auto &entry : pages->values) {
        std::cout << entry.first << ": " << entry.second->type << " | " << entry.second << std::endl;
    }
    std::cout << std::endl;
}
