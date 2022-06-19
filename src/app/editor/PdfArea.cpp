#include "PdfArea.h"

#include <gtkmm/checkbutton.h>
#include <gtkmm/eventcontrollermotion.h>

#include <pdf/renderer.h>

PdfArea::PdfArea(BaseObjectType *obj, const Glib::RefPtr<Gtk::Builder> & /*builder*/, pdf::Document &_document)
    : ScrolledZoomedContent(obj), document(_document) {
    set_draw_func(sigc::mem_fun(*this, &PdfArea::on_draw));

    auto motionCtrl = Gtk::EventControllerMotion::create();
    motionCtrl->signal_motion().connect(sigc::mem_fun(*this, &PdfArea::mouse_moved));
    add_controller(motionCtrl);

    int i             = 0;
    double pageOffset = 0.0;
    document.for_each_page([this, &i, &pageOffset](pdf::Page *page) {
        spdlog::info("Page {} with offset {}:", i++, pageOffset);

        pageTextBlocks.emplace_back(pageOffset, page->text_blocks());
        for (const auto &textBlock : pageTextBlocks.back().textBlocks) {
            spdlog::info("TextBlock: {},{} - {},{}", textBlock.x, textBlock.y, textBlock.width, textBlock.height);
        }

        pageOffset += page->height() + PAGE_PADDING;
        return pdf::ForEachResult::CONTINUE;
    });
}

void PdfArea::on_draw(const Cairo::RefPtr<Cairo::Context> &cr, int width, int height) {
    spdlog::trace("PdfArea::on_draw(width={}, height={})", width, height);

    cr->translate(-scrollOffsetX, -scrollOffsetY);
    cr->scale(_zoom, _zoom);

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
    cr->set_source_rgba(0, 0, 1, 0.1);

    for (const auto &pageTextBlock : pageTextBlocks) {
        auto verticalOffset = pageTextBlock.pageOffset - scrollOffsetY;
        for (const auto &textBlock : pageTextBlock.textBlocks) {
            graphene_rect_t r = {
                  {static_cast<float>(textBlock.x),                     //
                   static_cast<float>(textBlock.y - textBlock.height)}, //
                  {static_cast<float>(textBlock.width),                 //
                   static_cast<float>(textBlock.height)},               //
            };

            graphene_rect_t scaledRect = {};
            graphene_rect_scale(&r, static_cast<float>(_zoom), static_cast<float>(_zoom), &scaledRect);
            graphene_rect_offset(&scaledRect, -scrollOffsetX, verticalOffset);

            graphene_point_t p = {static_cast<float>(mouseX), static_cast<float>(mouseY)};
            if (!graphene_rect_contains_point(&scaledRect, &p)) {
                continue;
            }

            cr->rectangle(                                                   //
                  textBlock.x,                                               //
                  textBlock.y - textBlock.height + pageTextBlock.pageOffset, //
                  textBlock.width,                                           //
                  textBlock.height                                           //
            );
            cr->fill();
        }
    }
}

void PdfArea::set_offsets(const double x, const double y) {
    scrollOffsetX = x;
    scrollOffsetY = y;
    queue_draw();
}

void PdfArea::update_zoom(double z) {
    // TODO make zoom speed adapt with the current zoom level
    _zoom += z * 0.1;
    if (_zoom <= 0.1) {
        _zoom = 0.1;
    } else if (_zoom > 10.0) {
        _zoom = 10.0;
    }
    queue_draw();
}

void PdfArea::mouse_moved(double x, double y) {
    mouseX = x;
    mouseY = y;
    queue_draw();
}
