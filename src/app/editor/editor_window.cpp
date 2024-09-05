#include "editor_window.h"

#include <spdlog/spdlog.h>

EditorWindow::EditorWindow(GtkBuilder *builder_, AdwApplicationWindow *window_, pdf::Document document_)
    : builder(builder_), window(window_), document(std::move(document_)) {
    gtk_window_set_title(GTK_WINDOW(window), document.file.path.c_str());
    gtk_application_window_set_show_menubar(GTK_APPLICATION_WINDOW(window), true);

    pdf_window = GTK_SCROLLED_WINDOW(gtk_builder_get_object(builder, "PdfWindow"));
    pdf_area   = GTK_DRAWING_AREA(gtk_builder_get_object(builder, "PdfArea"));

    init_pdf_window();
    init_pdf_area();

    update_container_size();

    // TODO pdfWindow->document_changed_signal().connect(sigc::mem_fun(*this, &EditorWindow::on_document_change));
}

void EditorWindow::init_pdf_window() {
    const auto hadjustment = gtk_scrolled_window_get_hadjustment(pdf_window);
    g_signal_connect(hadjustment, "value-changed", G_CALLBACK(&EditorWindow::scroll_value_changed), this);
    const auto vadjustment = gtk_scrolled_window_get_vadjustment(pdf_window);
    g_signal_connect(vadjustment, "value-changed", G_CALLBACK(&EditorWindow::scroll_value_changed), this);

    const auto key_ctrl = gtk_event_controller_key_new();
    g_signal_connect(key_ctrl, "key-pressed", G_CALLBACK(&EditorWindow::on_key_pressed), this);
    g_signal_connect(key_ctrl, "key-released", G_CALLBACK(&EditorWindow::on_key_released), this);
    gtk_widget_add_controller(GTK_WIDGET(pdf_window), key_ctrl);

    const auto scroll_ctrl = gtk_event_controller_scroll_new(GTK_EVENT_CONTROLLER_SCROLL_VERTICAL);
    g_signal_connect(scroll_ctrl, "scroll", G_CALLBACK(&EditorWindow::on_scroll), this);
    gtk_widget_add_controller(GTK_WIDGET(pdf_window), scroll_ctrl);

    const auto drop_target = gtk_drop_target_new(GDK_TYPE_FILE_LIST, GDK_ACTION_COPY);
    g_signal_connect(drop_target, "enter", G_CALLBACK(&EditorWindow::on_dnd_enter), this);
    g_signal_connect(drop_target, "motion", G_CALLBACK(&EditorWindow::on_dnd_motion), this);
    g_signal_connect(drop_target, "drop", G_CALLBACK(&EditorWindow::on_dnd_drop), this);
    g_signal_connect(drop_target, "leave", G_CALLBACK(&EditorWindow::on_dnd_leave), this);
    gtk_widget_add_controller(GTK_WIDGET(pdf_window), GTK_EVENT_CONTROLLER(drop_target));
}

void EditorWindow::init_pdf_area() {
    gtk_drawing_area_set_draw_func(pdf_area, (GtkDrawingAreaDrawFunc)&EditorWindow::draw_pdf_func, this, nullptr);

    const auto motion_ctrl = gtk_event_controller_motion_new();
    g_signal_connect(motion_ctrl, "motion", G_CALLBACK(&EditorWindow::on_mouse_moved), this);
    gtk_widget_add_controller(GTK_WIDGET(pdf_area), motion_ctrl);

    const auto drag_ctrl = gtk_gesture_drag_new();
    g_signal_connect(drag_ctrl, "drag-begin", G_CALLBACK(&EditorWindow::on_mouse_drag_begin), this);
    g_signal_connect(drag_ctrl, "drag-update", G_CALLBACK(&EditorWindow::on_mouse_drag_update), this);
    g_signal_connect(drag_ctrl, "drag-end", G_CALLBACK(&EditorWindow::on_mouse_drag_end), this);
    gtk_widget_add_controller(GTK_WIDGET(pdf_area), GTK_EVENT_CONTROLLER(drag_ctrl));
}

std::pair<double, double> calculate_content_size(pdf::Document &document) {
    // TODO: (hmueller) potentially move this function into the pdf library
    int page_count = 0;
    double width   = 0;
    double height  = 0;
    document.for_each_page([&page_count, &width, &height](pdf::Page *page) {
        page_count++;

        double currentWidth = page->attr_width();
        if (currentWidth > width) {
            width = currentWidth;
        }

        height += page->attr_height();
        return pdf::ForEachResult::CONTINUE;
    });

    return {width, height + (page_count - 1) * PAGE_PADDING};
}

void EditorWindow::update_container_size() {
    auto size   = calculate_content_size(document);
    auto width  = static_cast<int>(size.first * zoom);
    auto height = static_cast<int>(size.second * zoom);

    spdlog::info("{} width={} height={}", __FUNCTION__, width, height);
    gtk_drawing_area_set_content_width(pdf_area, width);
    gtk_drawing_area_set_content_height(pdf_area, height);
}

void EditorWindow::scroll_value_changed(GtkAdjustment *, EditorWindow *self) {
    spdlog::info(__FUNCTION__);
    gtk_widget_queue_draw(GTK_WIDGET(self->pdf_area));
}

bool EditorWindow::on_key_pressed(GtkEventControllerKey *, guint keyval, guint, GdkModifierType, EditorWindow *self) {
    spdlog::info(__FUNCTION__);
    if (keyval == GDK_KEY_Control_L) {
        self->is_ctrl_key_down = true;
    }
    return false;
}

void EditorWindow::on_key_released(GtkEventControllerKey *, guint keyval, guint, GdkModifierType, EditorWindow *self) {
    spdlog::info(__FUNCTION__);
    if (keyval == GDK_KEY_Control_L) {
        self->is_ctrl_key_down = false;
    }
}

bool EditorWindow::on_scroll(GtkEventControllerScroll *, gdouble, gdouble y, EditorWindow *self) {
    spdlog::info(__FUNCTION__);

    if (not self->is_ctrl_key_down) {
        return false;
    }

    // TODO make zoom speed adapt with the current zoom level
    self->zoom += y * -0.1;
    if (self->zoom <= 0.1) {
        self->zoom = 0.1;
    } else if (self->zoom > 10.0) {
        self->zoom = 10.0;
    }

    self->update_container_size();

    gtk_widget_queue_draw(GTK_WIDGET(self->pdf_area));

    return true;
}

GdkDragAction EditorWindow::on_dnd_enter(GtkDropTarget *, gdouble x, gdouble y, EditorWindow *) {
    spdlog::info("{}: x={} y={}", __FUNCTION__, x, y);
    return GDK_ACTION_COPY;
}

GdkDragAction EditorWindow::on_dnd_motion(GtkDropTarget *, gdouble x, gdouble y, EditorWindow *) {
    spdlog::info("{}: x={} y={}", __FUNCTION__, x, y);
    return GDK_ACTION_COPY;
}

void EditorWindow::on_dnd_leave(GtkDropTarget *, EditorWindow *) { spdlog::info(__FUNCTION__); }

gboolean EditorWindow::on_dnd_drop(GtkDropTarget *, const GValue *value, gdouble x, gdouble y, EditorWindow *self) {
    spdlog::info("{}: x={} y={}", __FUNCTION__, x, y);
    if (value->g_type != GDK_TYPE_FILE_LIST) {
        return false;
    }

    auto *fileList = reinterpret_cast<GdkFileList *>(g_value_get_boxed(value));
    GSList *list   = gdk_file_list_get_files(fileList);

    auto opened_file = false;
    for (GSList *l = list; l != nullptr; l = l->next) {
        auto *file    = reinterpret_cast<GFile *>(l->data);
        auto filePath = g_file_get_path(file);
        auto variant  = g_variant_new_string(filePath);
        gtk_widget_activate_action_variant(GTK_WIDGET(self->pdf_window), "app.open", variant);
        opened_file = true;
    }

    return opened_file;
}

void EditorWindow::on_mouse_moved(GtkEventControllerMotion *, double x, double y, EditorWindow *self) {
    self->mouse_x = x;
    self->mouse_y = y;
}

pdf::PageImage *EditorWindow::get_image_at_position(double x, double y) {
    for (auto &images : page_images) {
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

void EditorWindow::on_mouse_drag_begin(GtkGestureDrag *, gdouble offset_x, gdouble offset_y, EditorWindow *self) {
    spdlog::info(__FUNCTION__);

    auto image = self->get_image_at_position(offset_x, offset_y);
    if (image != nullptr) {
        self->drag_start_x = image->xOffset;
        self->drag_start_y = image->yOffset;
    }

    self->selected_image = image;
}

void EditorWindow::on_mouse_drag_update(GtkGestureDrag *, gdouble offset_x, gdouble offset_y, EditorWindow *self) {
    spdlog::info(__FUNCTION__);

    if (self->selected_image == nullptr) {
        return;
    }

    self->selected_image->xOffset = self->drag_start_x + offset_x;
    self->selected_image->yOffset = self->drag_start_y - offset_y;

    gtk_widget_queue_draw(GTK_WIDGET(self->pdf_area));
}

void EditorWindow::on_mouse_drag_end(GtkGestureDrag *, gdouble offset_x, gdouble offset_y, EditorWindow *self) {
    spdlog::info(__FUNCTION__);

    if (self->selected_image != nullptr) {
        const auto pageX = offset_x;
        const auto pageY = -offset_y;
        self->selected_image->move(self->document, pageX, pageY);
        // TODO: (hmueller) documentChangedSignal.emit();
    }

    self->drag_start_x = 0.0;
    self->drag_start_y = 0.0;

    gtk_widget_queue_draw(GTK_WIDGET(self->pdf_area));
}

void EditorWindow::draw_pdf_func(GtkDrawingArea *, cairo_t *cr, int width, int height, EditorWindow *self) {
    auto size                   = calculate_content_size(self->document);
    auto const &document_width  = size.first * self->zoom;
    auto const &document_height = size.second * self->zoom;
    if (width > document_width) {
        cairo_translate(cr, width / 2 - document_width / 2, 0);
    }

    spdlog::info("{}: width={} height={} document_width={} document_height={}", __FUNCTION__, width, height,
                 document_width, document_height);

    cairo_scale(cr, self->zoom, self->zoom);

    const auto &hadj        = gtk_scrolled_window_get_hadjustment(self->pdf_window);
    const auto &vadj        = gtk_scrolled_window_get_vadjustment(self->pdf_window);
    const auto x            = gtk_adjustment_get_value(hadj);
    const auto y            = gtk_adjustment_get_value(vadj);
    auto const visible_area = VisibleArea{.x = x, .y = y, .width = width, .height = height};
    self->render_pages(cr, visible_area);

    self->render_text_highlight(cr, visible_area);
    self->render_image_highlight(cr);
}

void EditorWindow::render_pages(cairo_t *cr, const VisibleArea &visible_area) {
    cairo_save(cr);

    auto const &scroll_offset_y = visible_area.y;

    auto position_y             = 0.0;
    const auto area_height      = gtk_widget_get_height(GTK_WIDGET(pdf_area));
    const auto screen_end_pos_y = scroll_offset_y + area_height;
    const auto pages            = document.pages();
    for (auto page : pages) {
        const auto pageHeight = page->attr_height();

        if ((scroll_offset_y >= position_y && scroll_offset_y <= position_y + pageHeight) ||
            (screen_end_pos_y >= position_y && screen_end_pos_y <= position_y + pageHeight)) {
            cairo_save(cr);
            page->render(cr);
            cairo_restore(cr);
        }

        auto dy = pageHeight + PAGE_PADDING;
        position_y += dy;
        cairo_translate(cr, 0, dy);
    }

    cairo_restore(cr);
}

void EditorWindow::render_text_highlight(cairo_t *cr, const VisibleArea &visible_area) {
    cairo_save(cr);

    cairo_set_source_rgba(cr, 0, 0, 1, 0.1);

    const auto &scroll_offset_x = visible_area.x;
    const auto &scroll_offset_y = visible_area.y;
    for (const auto &pageTextBlock : page_text_blocks) {
        auto verticalOffset = pageTextBlock.pageOffset - scroll_offset_y;
        for (const auto &textBlock : pageTextBlock.textBlocks) {
            graphene_rect_t r = {
                  {static_cast<float>(textBlock.x),                     //
                   static_cast<float>(textBlock.y - textBlock.height)}, //
                  {static_cast<float>(textBlock.width),                 //
                   static_cast<float>(textBlock.height)},               //
            };

            spdlog::info("       Rect: {} | {} | {} | {}", r.origin.x, r.origin.y, r.size.width, r.size.height);

            graphene_rect_t scaledRect = {};
            graphene_rect_scale(&r, static_cast<float>(zoom), static_cast<float>(zoom), &scaledRect);
            graphene_rect_offset(&scaledRect, static_cast<float>(-scroll_offset_x), static_cast<float>(verticalOffset));

            spdlog::info("Scaled Rect: {} | {} | {} | {}", scaledRect.origin.x, scaledRect.origin.y,
                         scaledRect.size.width, scaledRect.size.height);

            graphene_point_t p = {static_cast<float>(mouse_x), static_cast<float>(mouse_y)};

            spdlog::info("Mouse: {} | {}", p.x, p.y);
            if (!graphene_rect_contains_point(&scaledRect, &p)) {
                continue;
            }

            cairo_rectangle(cr,                                                        //
                            textBlock.x,                                               //
                            textBlock.y - textBlock.height + pageTextBlock.pageOffset, //
                            textBlock.width,                                           //
                            textBlock.height                                           //
            );
        }
    }

    cairo_fill(cr);

    cairo_restore(cr);
}

void EditorWindow::render_image_highlight(cairo_t *cr) {
    cairo_save(cr);

    for (const auto &images : page_images) {
        cairo_save(cr);

        auto pageHeight       = images.page->attr_height();
        cairo_matrix_t matrix = {};
        cairo_matrix_init(&matrix, 1.0, 0.0, 0.0, -1.0, 0.0, pageHeight);
        cairo_transform(cr, &matrix);

        for (const auto &image : images.images) {
            if (&image != selected_image) {
                continue;
            }

            cairo_rectangle(cr,                                        //
                            image.xOffset,                             //
                            image.yOffset,                             //
                            static_cast<double>(image.image->width()), //
                            static_cast<double>(image.image->height()) //
            );
            cairo_set_source_rgba(cr, 0, 1, 1, 0.1);
            cairo_fill(cr);
        }

        cairo_restore(cr);
    }

    cairo_restore(cr);
}

#if 0
// TODO: (hmueller) 
void EditorWindow::save() {
    auto path = document.file.path;
    if (path.empty()) {
        spdlog::info("No file path available for current document");
        return;
    }

    auto extension = path.substr(path.size() - 4);
    if (extension == ".pdf") {
        path = path.substr(0, path.size() - 4);
    }
    path = path + "_copy.pdf";

    spdlog::info("Saving {}", path);
    auto result = document.write_to_file(path);
    if (result.has_error()) {
        spdlog::warn("Failed to save document: {}", result.message());
        return;
    }

    set_title(path);
}

void EditorWindow::on_document_change() { set_title("* " + document.file.path); }
#endif
