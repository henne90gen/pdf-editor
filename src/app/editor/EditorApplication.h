#pragma once

#include <adwaita.h>
#include <memory>
#include <pdf/document.h>

#include "EditorWindow.h"

class EditorApplication {
  public:
    static std::shared_ptr<EditorApplication> create();

    EditorApplication(AdwApplication *app);

    static void on_activate(GtkApplication *, EditorApplication *self);
    static void on_startup(GtkApplication *, EditorApplication *self);
    static void on_open(GtkApplication *, GFile **files, gint num_files, gchar *, EditorApplication *self);
    static void on_open_file(GSimpleAction *, GVariant *parameter, EditorApplication *self);

    int run(int argc, char *argv[]) { return g_application_run(G_APPLICATION(application), argc, argv); }

    void open_window(const std::string &file_path);

  private:
    AdwApplication *application;
    GtkBuilder *builder;
    GtkFileDialog *dialog;
    std::unordered_map<std::string, std::unique_ptr<EditorWindow>> windows;

    // TODO static void on_open_file(EditorApplication *self, const Glib::VariantBase &);
    static void on_open_file_dialog(EditorApplication *self);
    static void on_open_file_dialog_response(EditorApplication *self, int response);
    static void on_save(EditorApplication *self);
};
