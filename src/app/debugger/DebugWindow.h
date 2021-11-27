#pragma once

// TODO this is a hack to get the gtkmm4 code to compile on Windows
#undef WIN32
#include <gtkmm/applicationwindow.h>
#include <gtkmm/builder.h>
#include <gtkmm/button.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/drawingarea.h>
#include <gtkmm/label.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/treestore.h>
#include <gtkmm/viewport.h>

#include <pdf/document.h>

#include "ContentWindow.h"
#include "DocumentTree.h"
#include "JumpToByteDialog.h"

class DebugWindow : public Gtk::ApplicationWindow {
  public:
    [[maybe_unused]] DebugWindow(BaseObjectType *obj, const Glib::RefPtr<Gtk::Builder> &builder,
                                 const std::string &filePath);

    void update_selected_byte_label(int b);
    void update_hovered_byte_label(int b);
    void update_memory_usage_label();
    void update_details_label(pdf::Object *object);
    void open_jump_to_byte_dialog();
    void response_jump_to_byte_dialog(int response);

  private:
    pdf::Document document;

    Gtk::Label *selectedByteLabel      = nullptr;
    Gtk::Label *hoveredByteLabel       = nullptr;
    Gtk::Label *memoryUsageLabel       = nullptr;
    Gtk::Label *detailsLabel           = nullptr;
    Gtk::CheckButton *trailerHighlight = nullptr;
    Gtk::CheckButton *objectsHighlight = nullptr;
    Gtk::Button *jumpToByteButton      = nullptr;
    Gtk::Button *parseDocumentButton   = nullptr;

    ContentArea *contentArea           = nullptr;
    DocumentTree *documentTree         = nullptr;
    JumpToByteDialog *jumpToByteDialog = nullptr;
};
