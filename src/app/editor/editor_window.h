#pragma once

#include <adwaita.h>
#include <filesystem>
#include <gtk/gtk.h>
#include <iostream>
#include <pdf/document.h>
#include <pdf/page.h>

constexpr int PAGE_PADDING = 10;

class EditorApplication;

struct PageTextBlocks {
    double pageOffset                      = 0.0;
    std::vector<pdf::TextBlock> textBlocks = {};

    PageTextBlocks(double _pageOffset, std::vector<pdf::TextBlock> _textBlocks)
        : pageOffset(_pageOffset), textBlocks(std::move(_textBlocks)) {}
};

struct PageImages {
    pdf::Page *page;
    std::vector<pdf::PageImage> images = {};
};

struct VisibleArea {
    double x   = 0.0;
    double y   = 0.0;
    int width  = 0;
    int height = 0;
};

class EditorWindow {
  public:
    EditorWindow(GtkBuilder *builder, AdwApplicationWindow *window, pdf::Document document);

    // TODO void save();

    void present() { gtk_window_present(GTK_WINDOW(window)); }

    static void scroll_value_changed(GtkAdjustment *adjustment, EditorWindow *self);
    static bool on_key_pressed(GtkEventControllerKey *, guint keyval, guint keycode, GdkModifierType state,
                               EditorWindow *self);
    static void on_key_released(GtkEventControllerKey *, guint keyval, guint keycode, GdkModifierType state,
                                EditorWindow *self);
    static bool on_scroll(GtkEventControllerScroll *, gdouble dx, gdouble dy, EditorWindow *self);
    static GdkDragAction on_dnd_enter(GtkDropTarget *, gdouble x, gdouble y, EditorWindow *self);
    static GdkDragAction on_dnd_motion(GtkDropTarget *, gdouble x, gdouble y, EditorWindow *self);
    static gboolean on_dnd_drop(GtkDropTarget *, const GValue *value, gdouble x, gdouble y, EditorWindow *self);
    static void on_dnd_leave(GtkDropTarget *, EditorWindow *self);
    static void on_mouse_moved(GtkEventControllerMotion *, double x, double y, EditorWindow *self);
    static void on_mouse_drag_begin(GtkGestureDrag *, gdouble offset_x, gdouble offset_y, EditorWindow *self);
    static void on_mouse_drag_update(GtkGestureDrag *, gdouble offset_x, gdouble offset_y, EditorWindow *self);
    static void on_mouse_drag_end(GtkGestureDrag *, gdouble offset_x, gdouble offset_y, EditorWindow *self);
    static void draw_pdf_func(GtkDrawingArea *area, cairo_t *cr, int width, int height, EditorWindow *self);

  private:
    void init_pdf_window();
    void init_pdf_area();
    void update_container_size();

    void render_pages(cairo_t *cr, const VisibleArea& visible_area);
    void render_text_highlight(cairo_t *cr, const VisibleArea& visible_area);
    void render_image_highlight(cairo_t *cr);

    pdf::PageImage *get_image_at_position(double x, double y);

  private:
    GtkBuilder *builder;
    AdwApplicationWindow *window;
    GtkScrolledWindow *pdf_window;
    GtkDrawingArea *pdf_area;

    pdf::Document document;

    std::vector<PageTextBlocks> page_text_blocks = {};
    std::vector<PageImages> page_images          = {};

    pdf::PageImage *selected_image = nullptr;

    bool is_ctrl_key_down = false;
    float zoom            = 1.0;
    float mouse_x         = 0.0;
    float mouse_y         = 0.0;
    float drag_start_x    = 0.0;
    float drag_start_y    = 0.0;

    // TODO void on_document_change();
};
