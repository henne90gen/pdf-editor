#include "PdfArea.h"

#include <gtkmm/checkbutton.h>

#include <pdf/renderer.h>

PdfArea::PdfArea(BaseObjectType *obj, const Glib::RefPtr<Gtk::Builder> &builder, pdf::Document &_document)
    : Gtk::DrawingArea(obj), document(_document) {
    set_draw_func(sigc::mem_fun(*this, &PdfArea::on_draw));

    auto showTextButton = builder->get_widget<Gtk::CheckButton>("HighlightTextCheckButton");
    showTextButton->signal_toggled().connect(sigc::mem_fun(*this, &PdfArea::on_highlight_text_toggled));
}

void PdfArea::on_draw(const Cairo::RefPtr<Cairo::Context> &cr, int width, int height) {
    spdlog::trace("PdfArea::on_draw(width={}, height={})", width, height);

    cr->translate(-offsetX, -offsetY);
    cr->scale(zoom, zoom);

    render_pages(cr);
    render_text_blocks(cr);
}

void PdfArea::render_pages(const Cairo::RefPtr<Cairo::Context> &cr) {
    cr->save();

    auto pages = document.pages();
    for (auto page : pages) {
        pdf::Renderer renderer(*page, cr);
        renderer.render();

        cr->translate(0, page->height() + PAGE_PADDING);
    }

    cr->restore();
}

void PdfArea::render_text_blocks(const Cairo::RefPtr<Cairo::Context> &cr) {
    if (!highlightText || textBlocks.empty()) {
        return;
    }

    cr->set_source_rgba(0, 0, 1, 0.1);
    for (auto &textBlock : textBlocks) {
        cr->rectangle(textBlock.x, textBlock.y - textBlock.height, textBlock.width, textBlock.height);
        cr->fill();
    }
}

void PdfArea::set_offsets(const double x, const double y) {
    offsetX = x;
    offsetY = y;
    queue_draw();
}

void PdfArea::update_zoom(double z) {
    // TODO make zoom speed adapt with the current zoom level
    zoom += z * 0.1;
    if (zoom <= 0.1) {
        zoom = 0.1;
    } else if (zoom > 10.0) {
        zoom = 10.0;
    }
    queue_draw();
}

void PdfArea::on_highlight_text_toggled() {
    highlightText = !highlightText;

    if (highlightText && textBlocks.empty()) {
        document.for_each_page([this](pdf::Page *page) {
            textBlocks = page->text_blocks();
            return pdf::ForEachResult::CONTINUE;
        });
    }

    queue_draw();
}
