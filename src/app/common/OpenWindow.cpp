#include "OpenWindow.h"

OpenWindow::OpenWindow(BaseObjectType *obj, const Glib::RefPtr<Gtk::Builder> &builder,
                       std::function<void(const std::string &)> _func)
    : Gtk::ApplicationWindow(obj), func(std::move(_func)) {
    titlebar = builder->get_widget<Gtk::HeaderBar>("TitleBar");
    set_titlebar(*titlebar);
    openButton = builder->get_widget<Gtk::Button>("OpenButton");
    openButton->signal_clicked().connect(sigc::mem_fun(*this, &OpenWindow::on_open_file));
}

void OpenWindow::on_open_response(int response) {
    if (response == Gtk::ResponseType::ACCEPT) {
        auto file = openDialog->get_file();
        func(file->get_path());
        delete openDialog;
        close();
        return;
    }

    delete openDialog;
}

void OpenWindow::on_open_file() {
    if (openDialog != nullptr) {
        return;
    }

    openDialog = new Gtk::FileChooserDialog(*this, "Open Document", Gtk::FileChooserDialog::Action::OPEN, true);
    openDialog->set_select_multiple(false);
    openDialog->set_modal(true);
    openDialog->add_button("Open", Gtk::ResponseType::ACCEPT);
    openDialog->add_button("Cancel", Gtk::ResponseType::CANCEL);
    openDialog->signal_response().connect(sigc::mem_fun(*this, &OpenWindow::on_open_response));
    openDialog->show();
}
