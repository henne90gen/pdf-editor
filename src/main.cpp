#include <iostream>

#include <gtkmm/applicationwindow.h>
#include <gtkmm/builder.h>
#include <gtkmm/button.h>

class MainWindow : public Gtk::ApplicationWindow {
public:
  MainWindow(BaseObjectType *obj, Glib::RefPtr<Gtk::Builder> _builder)
      : Gtk::ApplicationWindow(obj), builder(std::move(_builder)) {
    Gtk::Button *helloBtn;
    builder->get_widget("HelloBtn", helloBtn);
    helloBtn->signal_clicked().connect(
        sigc::mem_fun(this, &MainWindow::on_hello_clicked));
  }

  ~MainWindow() override = default;

protected:
  void on_hello_clicked() { std::cout << "Hello World" << std::endl; }

private:
  Glib::RefPtr<Gtk::Builder> builder;
};

int main(int argc, char *argv[]) {
  auto app = Gtk::Application::create(argc, argv, "de.henne90gen.pdf_editor");
  auto builder = Gtk::Builder::create_from_file("../../src/pdf-editor.glade");

  MainWindow *wnd = nullptr;
  builder->get_widget_derived("MainWindow", wnd);
  auto r = app->run(*wnd);
  delete wnd;
  return r;
}
