#pragma once

#include <filesystem>
#include <iostream>
#include <spdlog/spdlog.h>

#include <glibmm/convert.h>
#include <glibmm/markup.h>
#include <gtkmm/applicationwindow.h>
#include <gtkmm/builder.h>
#include <gtkmm/notebook.h>
#include <gtkmm/textview.h>

#include "PdfPage.h"

class EditorWindow : public Gtk::ApplicationWindow {
  public:
    [[maybe_unused]] EditorWindow(BaseObjectType *obj, Glib::RefPtr<Gtk::Builder> _builder);
    ~EditorWindow() override = default;

    void add_document(const std::string &filePath);

  protected:
    //    bool custom_on_drag_drop(const Glib::RefPtr<Gdk::DragContext> &context, int x, int y, guint time);
    //
    //    void custom_on_drag_data_received(const Glib::RefPtr<Gdk::DragContext> &context, int x, int y,
    //                                      const Gtk::SelectionData &selection_data, guint info, guint time);

  private:
    Glib::RefPtr<Gtk::Builder> builder;
    Gtk::Notebook *notebook = nullptr;

    // using std::list here, because the stored objects have to have stable addresses
    std::list<pdf::Document> documents = {};
    std::list<PdfPage> pages           = {};
};
