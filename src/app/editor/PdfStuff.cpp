#include "PdfStuff.h"

#include <pdf/renderer.h>
#include <spdlog/spdlog.h>

#if 0

PdfPage::PdfPage(pdf::Document &_file) : pdfWidget(_file) {
    box.set_orientation(Gtk::Orientation::HORIZONTAL);
    set_child(box);

    rightScrolledWindow.set_child(pdfWidget);

    box.append(leftScrolledWindow);
    box.append(rightScrolledWindow);

    show();
}

PdfWidget::PdfWidget(pdf::Document &_file)
    : Gtk::Viewport(Glib::RefPtr<Gtk::Adjustment>(), Glib::RefPtr<Gtk::Adjustment>()), file(_file) {
    this->set_hadjustment(hadjustment);
    this->set_vadjustment(vadjustment);

    set_can_focus(true);
    set_focus_on_click(true);

    drawingArea.set_draw_func(sigc::mem_fun(*this, &PdfWidget::on_draw));
    //    add_events(Gdk::SMOOTH_SCROLL_MASK | Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK | Gdk::KEY_PRESS_MASK |
    //               Gdk::KEY_RELEASE_MASK);
    set_child(drawingArea);
}

void PdfWidget::on_draw(const Cairo::RefPtr<Cairo::Context> &cr, int width, int height) {
    spdlog::trace("PdfWidget::on_draw(width={}, height={})", width, height);

    cr->scale(zoom, zoom);

    auto pages = file.pages();
    for (auto page : pages) {
        pdf::Renderer renderer(*page);
        // FIXME undefined reference to `pdf::renderer::render(std::shared_ptr<Cairo::Context> const&)'
        //  renderer.render(cr);

        int padding = 10;
        cr->translate(0, page->height() + padding);
    }
}

// bool PdfWidget::on_scroll_event(GdkEventScroll *event) {
//     spdlog::trace("PdfWidget: scroll event");
//     if (isCtrlPressed) {
//         zoom -= event->delta_y * zoomSpeed;
//         if (zoom <= 0.1) {
//             zoom = 0.1;
//         }
//
//         Gtk::Allocation allocation = get_allocation();
//         update_adjustments(allocation);
//         queue_draw();
//
//         return true;
//     }
//
//     return false;
// }
//
// bool PdfWidget::on_key_press_event(GdkEventKey *key_event) {
//     spdlog::trace("PdfWidget: key press event");
//     if (key_event->keyval == GDK_KEY_Control_L) {
//         isCtrlPressed = true;
//     }
//     return false;
// }
//
// bool PdfWidget::on_key_release_event(GdkEventKey *key_event) {
//     spdlog::trace("PdfWidget: key release event");
//     if (key_event->keyval == GDK_KEY_Control_L) {
//         isCtrlPressed = false;
//     }
//     return false;
// }
//
// bool PdfWidget::on_button_press_event(GdkEventButton *button_event) {
//     spdlog::trace("PdfWidget: button press event");
//     grab_focus();
//     return false;
// }

void PdfWidget::update_adjustments(int w, int h) {
    double width  = 0;
    double height = 0;
    auto pages    = file.pages();
    for (auto page : pages) {
        double currentWidth = page->width();
        if (currentWidth > width) {
            width = currentWidth;
        }
        // TODO maybe add some padding between the pages
        height += page->height();
    }

    auto scaled_width  = width * zoom;
    auto scaled_height = height * zoom;

    auto page_size_x = w * 1.0 / scaled_width;
    hadjustment->configure(hadjustment->get_value(), 0.0, 1.0, 0.1, 0.5, page_size_x);

    auto page_size_y = h * 1.0 / scaled_height;
    vadjustment->configure(vadjustment->get_value(), 0.0, 1.0, 0.1, 0.5, page_size_y);

    drawingArea.set_size_request(static_cast<int>(width), static_cast<int>(height));
}

void PdfWidget::size_allocate_vfunc(int w, int h, int baseline) {
    spdlog::trace("PdfWidget: size allocate");
    update_adjustments(w, h);
    Viewport::size_allocate_vfunc(w, h, baseline);
}

#endif
