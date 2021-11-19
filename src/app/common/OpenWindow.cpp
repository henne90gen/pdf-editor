#include "OpenWindow.h"

OpenWindow::OpenWindow(BaseObjectType *obj, const Glib::RefPtr<Gtk::Builder> &builder,
                       const std::function<void(const std::string &)> &_func)
    : Gtk::ApplicationWindow(obj), func(_func) {
    titlebar = builder->get_widget<Gtk::HeaderBar>("TitleBar");
    set_titlebar(*titlebar);
    openButton = builder->get_widget<Gtk::Button>("OpenButton");
    openButton->signal_clicked().connect(sigc::mem_fun(*this, &OpenWindow::open_document));
}

void OpenWindow::open_document() {
    func("Hello world");
}
