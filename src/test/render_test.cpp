#include <gtest/gtest.h>

#include <pdf/document.h>
#include <pdf/renderer.h>

TEST(Renderer, Blank) {
    pdf::Document document;
    pdf::Document::load_from_file("../../../test-files/blank.pdf", document);

    auto surface = Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, 100, 100);
    auto cr      = Cairo::Context::create(surface);

    auto pages = document.pages();
    for (auto page : pages) {
        pdf::renderer renderer(page);
        renderer.render(cr);
    }
}

TEST(Renderer, HelloWorld) {
    pdf::Document document;
    pdf::Document::load_from_file("../../../test-files/hello-world.pdf", document);

    auto surface = Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, 1000, 1000);
    auto cr      = Cairo::Context::create(surface);

    auto pages = document.pages();
    for (auto page : pages) {
        pdf::renderer renderer(page);
        renderer.render(cr);
    }

    surface->write_to_png("test.png");
}
