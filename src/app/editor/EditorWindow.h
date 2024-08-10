#pragma once

#include <adwaita.h>
#include <filesystem>
#include <iostream>
#include <pdf/document.h>

class EditorApplication;

class EditorWindow {
  public:
    EditorWindow(EditorApplication &app, GtkBuilder *builder, AdwApplicationWindow *window, pdf::Document &document);

    // TODO void save();

    void present() { gtk_window_present(GTK_WINDOW(window)); }

  private:
    // EditorApplication &app;
    // GtkBuilder *builder;
    AdwApplicationWindow *window;

    pdf::Document document;

    // TODO void on_document_change();
    // TODO Gdk::DragAction on_dnd_enter(double x, double y);
    // TODO Gdk::DragAction on_dnd_motion(double x, double y);
    // TODO bool on_dnd_drop(const Glib::ValueBase &value, double x, double y);
};
