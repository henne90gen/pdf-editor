#include "PdfPage.h"

#include <pdf/renderer.h>
#include <spdlog/spdlog.h>

PdfPage::PdfPage(pdf::Document &_file) : file(_file), pdfWidget(_file) {
    box.set_orientation(Gtk::Orientation::HORIZONTAL);
    set_child(box);

    leftScrolledWindow.set_child(treeView);

    rightScrolledWindow.set_child(pdfWidget);

    box.append(leftScrolledWindow);
    box.append(rightScrolledWindow);

    treeStore = Gtk::TreeStore::create(columns);
    treeView.set_model(treeStore);

    auto root = file.catalog();
    ASSERT(root != nullptr);
    //    addRows(root, 0);

    treeView.append_column("Name", columns.m_col_name);
    treeView.append_column("Value", columns.m_col_value);

    show();
}

void PdfPage::addRows(pdf::Object *obj, int depth, Gtk::TreeModel::Row *parentRow) {
    // TODO use a hash set to stop infinite recursion
    if (depth > 5) {
        return;
    }
    if (obj == nullptr) {
        // TODO maybe show a special row?
        return;
    }

    switch (obj->type) {
    case pdf::Object::Type::BOOLEAN: {
        ASSERT(parentRow != nullptr);
        if (obj->as<pdf::Boolean>()->value) {
            (*parentRow)[columns.m_col_value] = "true";
        } else {
            (*parentRow)[columns.m_col_value] = "false";
        }
        break;
    }
    case pdf::Object::Type::INTEGER: {
        ASSERT(parentRow != nullptr);
        (*parentRow)[columns.m_col_value] = std::to_string(obj->as<pdf::Integer>()->value);
        break;
    }
    case pdf::Object::Type::REAL: {
        ASSERT(parentRow != nullptr);
        (*parentRow)[columns.m_col_value] = std::to_string(obj->as<pdf::Real>()->value);
        break;
    }
    case pdf::Object::Type::HEXADECIMAL_STRING: {
        ASSERT(parentRow != nullptr);
        auto *hexString                   = obj->as<pdf::HexadecimalString>();
        (*parentRow)[columns.m_col_value] = std::string(hexString->data) + " (" + hexString->to_string() + ")";
        break;
    }
    case pdf::Object::Type::LITERAL_STRING: {
        ASSERT(parentRow != nullptr);
        (*parentRow)[columns.m_col_value] = std::string(obj->as<pdf::LiteralString>()->value());
        break;
    }
    case pdf::Object::Type::NAME: {
        ASSERT(parentRow != nullptr);
        (*parentRow)[columns.m_col_value] = std::string(obj->as<pdf::Name>()->value());
        break;
    }
    case pdf::Object::Type::ARRAY:
        addRows(obj->as<pdf::Array>(), depth, parentRow);
        break;
    case pdf::Object::Type::DICTIONARY:
        addRows(obj->as<pdf::Dictionary>(), depth, parentRow);
        break;
    case pdf::Object::Type::INDIRECT_REFERENCE:
        addRows(file.resolve(obj->as<pdf::IndirectReference>()), depth, parentRow);
        break;
    case pdf::Object::Type::INDIRECT_OBJECT:
        addRows(obj->as<pdf::IndirectObject>()->object, depth, parentRow);
        break;
    case pdf::Object::Type::STREAM:
        ASSERT(parentRow != nullptr);
        // TODO this does not yet work as expected (crashes with "munmap_chunk(): invalid pointer")
        //        (*parentRow)[columns.m_col_value] = std::string(obj->as<pdf::Stream>()->to_string());
        break;
    case pdf::Object::Type::NULL_OBJECT: {
        ASSERT(parentRow != nullptr);
        (*parentRow)[columns.m_col_value] = "null";
        break;
    }
    default:
        ASSERT(false);
    }
}

void PdfPage::addRows(const pdf::Dictionary *dict, int depth, Gtk::TreeModel::Row *parentRow) {
    for (auto &entry : dict->values) {
        auto row                = createRow(parentRow);
        row[columns.m_col_name] = entry.first;
        addRows(entry.second, depth + 1, &row);
    }
}

void PdfPage::addRows(const pdf::Array *arr, int depth, Gtk::TreeModel::Row *parentRow) {
    for (size_t i = 0; i < arr->values.size(); i++) {
        auto row                = createRow(parentRow);
        row[columns.m_col_name] = std::to_string(i);
        addRows(arr->values[i], depth + 1, &row);
    }
}

Gtk::TreeModel::Row PdfPage::createRow(Gtk::TreeRow *parentRow) {
    if (parentRow == nullptr) {
        return *treeStore->append();
    } else {
        return *treeStore->append(parentRow->children());
    }
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
        pdf::Renderer renderer(page);
        // FIXME undefined reference to `pdf::renderer::render(std::shared_ptr<Cairo::Context> const&)'
        //        renderer.render(cr);

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
