#include "PdfPage.h"

#include <pdf/renderer.h>
#include <spdlog/spdlog.h>

PdfPage::PdfPage(pdf::Document &_file) : file(_file), pdfWidget(_file) {
    box.set_orientation(Gtk::ORIENTATION_HORIZONTAL);
    add(box);

    leftScrolledWindow.add(treeView);

    rightScrolledWindow.set_policy(Gtk::POLICY_ALWAYS, Gtk::POLICY_ALWAYS);
    rightScrolledWindow.add(pdfWidget);

    box.pack_start(leftScrolledWindow, false, true);
    box.pack_start(rightScrolledWindow);

    treeStore = Gtk::TreeStore::create(columns);
    treeView.set_model(treeStore);

    auto root = file.root();
    ASSERT(root != nullptr);
//    addRows(root, 0);

    treeView.append_column("Name", columns.m_col_name);
    treeView.append_column("Value", columns.m_col_value);

    show_all_children();
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
        (*parentRow)[columns.m_col_value] = std::string(hexString->value) + " (" + hexString->to_string() + ")";
        break;
    }
    case pdf::Object::Type::LITERAL_STRING: {
        ASSERT(parentRow != nullptr);
        (*parentRow)[columns.m_col_value] = std::string(obj->as<pdf::LiteralString>()->value);
        break;
    }
    case pdf::Object::Type::NAME: {
        ASSERT(parentRow != nullptr);
        (*parentRow)[columns.m_col_value] = std::string(obj->as<pdf::Name>()->value);
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
    for (int i = 0; i < arr->values.size(); i++) {
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
    hadjustment->signal_value_changed().connect(sigc::mem_fun(*this, &PdfWidget::hadjustment_changed));
    vadjustment->signal_value_changed().connect(sigc::mem_fun(*this, &PdfWidget::vadjustment_changed));

    set_can_focus(true);
    set_focus_on_click(true);

    drawingArea.signal_draw().connect(sigc::mem_fun(*this, &PdfWidget::my_on_draw));
    add_events(Gdk::SMOOTH_SCROLL_MASK | Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK | Gdk::KEY_PRESS_MASK |
               Gdk::KEY_RELEASE_MASK);
    Gtk::Container::add(drawingArea);
}

bool PdfWidget::my_on_draw(const Cairo::RefPtr<Cairo::Context> &cr) {
    spdlog::trace("PdfWidget: draw");

    cr->scale(zoom, zoom);

    auto pages = file.pages();
    for (auto page : pages) {
        pdf::renderer renderer(page);
        renderer.render(cr);

        // TODO maybe add some padding between the pages
        cr->translate(0, page->height());
    }

    return false;
}

bool PdfWidget::on_scroll_event(GdkEventScroll *event) {
    spdlog::trace("PdfWidget: scroll event");
    if (isCtrlPressed) {
        zoom -= event->delta_y * zoomSpeed;
        if (zoom <= 0.1) {
            zoom = 0.1;
        }

        queue_draw();

        return true;
    }

    return false;
}

bool PdfWidget::on_key_press_event(GdkEventKey *key_event) {
    spdlog::trace("PdfWidget: key press event");
    if (key_event->keyval == GDK_KEY_Control_L) {
        isCtrlPressed = true;
    }
    return false;
}

bool PdfWidget::on_key_release_event(GdkEventKey *key_event) {
    spdlog::trace("PdfWidget: key release event");
    if (key_event->keyval == GDK_KEY_Control_L) {
        isCtrlPressed = false;
    }
    return false;
}

bool PdfWidget::on_button_press_event(GdkEventButton *button_event) {
    spdlog::trace("PdfWidget: button press event");
    grab_focus();
    return false;
}

void PdfWidget::update_adjustments(Gtk::Allocation &allocation) {
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

    auto page_size_x = allocation.get_width() * 1.0 / scaled_width;
    hadjustment->configure(hadjustment->get_value(), 0.0, 1.0, 0.1, 0.5, page_size_x);

    auto page_size_y = allocation.get_height() * 1.0 / scaled_height;
    vadjustment->configure(vadjustment->get_value(), 0.0, 1.0, 0.1, 0.5, page_size_y);

    drawingArea.set_size_request(static_cast<int>(width), static_cast<int>(height));
}

void PdfWidget::on_size_allocate(Gtk::Allocation &allocation) {
    spdlog::trace("PdfWidget: size allocate");
    update_adjustments(allocation);
    Viewport::on_size_allocate(allocation);
}
