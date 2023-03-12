#include "PdfArea.h"

#include <gtkmm/checkbutton.h>
#include <gtkmm/eventcontrollermotion.h>
#include <gtkmm/gestureclick.h>
#include <gtkmm/gesturedrag.h>

#include <pdf/renderer.h>

PdfArea::PdfArea(BaseObjectType *obj, const Glib::RefPtr<Gtk::Builder> & /*builder*/, pdf::Document &_document)
    : ScrolledZoomedContent(obj), document(_document) {
    set_draw_func(sigc::mem_fun(*this, &PdfArea::on_draw));

    auto dragCtrl = Gtk::GestureDrag::create();
    dragCtrl->signal_drag_begin().connect(sigc::mem_fun(*this, &PdfArea::on_mouse_drag_begin));
    dragCtrl->signal_drag_update().connect(sigc::mem_fun(*this, &PdfArea::on_mouse_drag_update));
    dragCtrl->signal_drag_end().connect(sigc::mem_fun(*this, &PdfArea::on_mouse_drag_end));
    add_controller(dragCtrl);

    auto motionCtrl = Gtk::EventControllerMotion::create();
    motionCtrl->signal_motion().connect(sigc::mem_fun(*this, &PdfArea::on_mouse_moved));
    add_controller(motionCtrl);

    int i             = 0;
    double pageOffset = 0.0;
    document.for_each_page([this, &i, &pageOffset](pdf::Page *page) {
        spdlog::info("Page {} with offset {}", i++, pageOffset);

        pageTextBlocks.emplace_back(pageOffset, page->text_blocks());
        for (const auto &textBlock : pageTextBlocks.back().textBlocks) {
            spdlog::info("Found TextBlock at position {},{} with size {}x{}", textBlock.x, textBlock.y, textBlock.width,
                         textBlock.height);
        }

        const auto &images = page->images();
        for (const auto &img : images) {
            spdlog::info("Found Image {} at position {},{} with size {}x{}", img.name, img.xOffset, img.yOffset,
                         img.image->width(), img.image->height());
        }
        pageImages.push_back({page, images});

        pageOffset += page->height() + PAGE_PADDING;
        return pdf::ForEachResult::CONTINUE;
    });
}

void PdfArea::on_draw(const Cairo::RefPtr<Cairo::Context> &cr, int /*width*/, int /*height*/) {
    //    spdlog::trace("PdfArea::on_draw(width={}, height={})", width, height);

    cr->translate(-scrollOffsetX, -scrollOffsetY);
    cr->scale(_zoom, _zoom);

    render_pages(cr);
    render_text_highlight(cr);
    render_image_highlight(cr);
}

void PdfArea::render_pages(const Cairo::RefPtr<Cairo::Context> &cr) {
    cr->save();

    auto pages = document.pages();
    for (auto page : pages) {
        cr->save();

        // move (0,0) from the top-left to the bottom-left corner of the page and make positive y-axis extend vertically
        // upward
        auto pageHeight = page->height();
        auto matrix     = Cairo::Matrix(1.0, 0.0, 0.0, -1.0, 0.0, pageHeight);
        cr->transform(matrix);

        pdf::Renderer renderer(*page, cr);
        renderer.render();

        cr->restore();

        cr->translate(0, pageHeight + PAGE_PADDING);
    }

    cr->restore();
}

void PdfArea::render_text_highlight(const Cairo::RefPtr<Cairo::Context> &cr) {
    cr->save();

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
            graphene_rect_offset(&scaledRect, static_cast<float>(-scrollOffsetX), static_cast<float>(verticalOffset));

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
        }
    }
    cr->fill();

    cr->restore();
}

void PdfArea::render_image_highlight(const Cairo::RefPtr<Cairo::Context> &cr) {
    cr->save();

    for (const auto &images : pageImages) {
        cr->save();

        auto pageHeight = images.page->height();
        auto matrix     = Cairo::Matrix(1.0, 0.0, 0.0, -1.0, 0.0, pageHeight);
        cr->transform(matrix);

        for (const auto &image : images.images) {
            cr->rectangle(                                   //
                  image.xOffset,                             //
                  image.yOffset,                             //
                  static_cast<double>(image.image->width()), //
                  static_cast<double>(image.image->height()) //
            );
            if (&image == selectedImage) {
                cr->set_source_rgba(0, 1, 1, 1);
            } else {
                cr->set_source_rgba(0, 0, 1, 0.1);
            }
            cr->fill();
        }

        cr->restore();
    }

    cr->restore();
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

void PdfArea::on_mouse_moved(double x, double y) {
    mouseX = x;
    mouseY = y;
    queue_draw();
}

pdf::PageImage *PdfArea::get_image_at_position(double x, double y) {
    for (auto &images : pageImages) {
        const auto pageX = x;
        const auto pageY = images.page->height() - y;

        for (auto &image : images.images) {
            if (pageX < image.xOffset || //
                pageX > image.xOffset + static_cast<double>(image.image->width())) {
                continue;
            }
            if (pageY < image.yOffset || //
                pageY > image.yOffset + static_cast<double>(image.image->height())) {
                continue;
            }

            return &image;
        }
    }

    return nullptr;
}

void PdfArea::on_mouse_drag_begin(double x, double y) {
    auto image = get_image_at_position(x, y);
    if (image != nullptr) {
        dragStartX = image->xOffset;
        dragStartY = image->yOffset;
    }
    selectedImage = image;
}

void PdfArea::on_mouse_drag_update(double x, double y) {
    if (selectedImage == nullptr) {
        return;
    }

    selectedImage->xOffset = dragStartX + x;
    selectedImage->yOffset = dragStartY - y;

    queue_draw();
}

void PdfArea::on_mouse_drag_end(double x, double y) {
    if (selectedImage != nullptr) {
        const auto pageX = x;
        const auto pageY = -y;
        selectedImage->move(document, pageX, pageY);
    }
    dragStartX = 0.0;
    dragStartY = 0.0;
    queue_draw();
}
