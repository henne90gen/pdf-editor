#include "PdfWindow.h"

PdfWindow::PdfWindow(BaseObjectType *obj, const Glib::RefPtr<Gtk::Builder> &builder, pdf::Document &_document)
    : ScrolledZoomedWindow(obj, builder, "PdfContainer"), document(_document) {
    pdfArea = Gtk::Builder::get_widget_derived<PdfArea>(builder, "PdfArea", document);

    // NOTE setting default focus to PdfWindow
    grab_focus();
    update_container_size();
}

std::pair<double, double> PdfWindow::calculate_content_size() {
    double width  = 0;
    double height = PAGE_PADDING;
    document.for_each_page([&width, &height](pdf::Page *page) {
        double currentWidth = page->attr_width();
        if (currentWidth > width) {
            width = currentWidth;
        }

        height += page->attr_height() + PAGE_PADDING;
        return pdf::ForEachResult::CONTINUE;
    });

    return {width, height};
}

ScrolledZoomedContent &PdfWindow::content() { return *pdfArea; }
