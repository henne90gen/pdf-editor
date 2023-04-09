#include "PdfArea.h"

#include <gtkmm/eventcontrollermotion.h>
#include <gtkmm/gestureclick.h>
#include <gtkmm/gesturedrag.h>

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

    init_document_data();
    documentChangedSignal.connect(sigc::mem_fun(*this, &PdfArea::init_document_data));
}

void PdfArea::init_document_data() {
    pageTextBlocks.clear();
    pageImages.clear();

    return;

    int i             = 0;
    double pageOffset = 0.0;
    document.for_each_page([&i, &pageOffset](pdf::Page *page) {
        spdlog::info("Page {} with offset {}", i++, pageOffset);

        // pageTextBlocks.emplace_back(pageOffset, page->text_blocks());
        // for (const auto &textBlock : pageTextBlocks.back().textBlocks) {
        //     spdlog::info("Found TextBlock at position {},{} with size {}x{}", textBlock.x, textBlock.y,
        //     textBlock.width,
        //                  textBlock.height);
        // }

        // const auto &images = page->images();
        // for (const auto &img : images) {
        //     spdlog::info("Found Image {} at position {},{} with size {}x{}", img.name, img.xOffset,
        //     img.yOffset,
        //                  img.image->width(), img.image->height());
        // }
        // pageImages.push_back({page, images});

        pageOffset += page->attr_height() + PAGE_PADDING;
        return pdf::ForEachResult::CONTINUE;
    });
}

void PdfArea::on_draw(const Cairo::RefPtr<Cairo::Context> &cr, int /*width*/, int /*height*/) {
    // spdlog::trace("PdfArea::on_draw(width={}, height={})", width, height);

    cr->translate(-scrollOffsetX, -scrollOffsetY);
    cr->scale(_zoom, _zoom);

    render_pages(cr);

    // render_text_highlight(cr);
    // render_image_highlight(cr);
}

void PdfArea::render_pages(const Cairo::RefPtr<Cairo::Context> &cr) {
    cr->save();

    auto positionY     = 0.0;
    auto pages         = document.pages();
    auto screenEndPosY = scrollOffsetY + get_height();
    for (auto page : pages) {
        auto pageHeight = page->attr_height();

        if ((scrollOffsetY >= positionY && scrollOffsetY <= positionY + pageHeight) ||
            (screenEndPosY >= positionY && screenEndPosY <= positionY + pageHeight)) {
            cr->save();
            page->render(cr);
            cr->restore();
        }

        auto dy = pageHeight + PAGE_PADDING;
        positionY += dy;
        cr->translate(0, dy);
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

            spdlog::info("       Rect: {} | {} | {} | {}", r.origin.x, r.origin.y, r.size.width, r.size.height);

            graphene_rect_t scaledRect = {};
            graphene_rect_scale(&r, static_cast<float>(_zoom), static_cast<float>(_zoom), &scaledRect);
            graphene_rect_offset(&scaledRect, static_cast<float>(-scrollOffsetX), static_cast<float>(verticalOffset));

            spdlog::info("Scaled Rect: {} | {} | {} | {}", scaledRect.origin.x, scaledRect.origin.y,
                         scaledRect.size.width, scaledRect.size.height);

            graphene_point_t p = {static_cast<float>(mouseX), static_cast<float>(mouseY)};

            spdlog::info("Mouse: {} | {}", p.x, p.y);
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

        auto pageHeight = images.page->attr_height();
        auto matrix     = Cairo::Matrix(1.0, 0.0, 0.0, -1.0, 0.0, pageHeight);
        cr->transform(matrix);

        for (const auto &image : images.images) {
            if (&image != selectedImage) {
                continue;
            }

            cr->rectangle(                                   //
                  image.xOffset,                             //
                  image.yOffset,                             //
                  static_cast<double>(image.image->width()), //
                  static_cast<double>(image.image->height()) //
            );
            cr->set_source_rgba(0, 1, 1, 0.1);
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
    //    queue_draw();
}

pdf::PageImage *PdfArea::get_image_at_position(double x, double y) {
    for (auto &images : pageImages) {
        const auto pageX = x;
        const auto pageY = images.page->attr_height() - y;

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
        documentChangedSignal.emit();
    }

    dragStartX = 0.0;
    dragStartY = 0.0;

    queue_draw();
}
