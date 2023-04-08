#include <gtest/gtest.h>

#include <pdf/document.h>

#if 0
TEST(Renderer, Blank) {
    pdf::Document document;
    pdf::Document::read_from_file("../../../test-files/blank.pdf", document);

    auto surface = Cairo::ImageSurface::create(Cairo::ImageSurface::Format::ARGB32, 100, 100);
    auto cr      = Cairo::Context::create(surface);

    auto pages = document.pages();
    for (auto page : pages) {
        pdf::Renderer renderer(*page, cr);
        renderer.render();
    }
}

TEST(Renderer, HelloWorld) {
    pdf::Document document;
    pdf::Document::read_from_file("../../../test-files/hello-world.pdf", document);

    auto pages = document.pages();
    for (size_t i = 0; i < pages.size(); i++) {
        auto page    = pages[i];
        auto cropBox = page->crop_box();
        auto width   = cropBox->width();
        auto height  = cropBox->height();
        auto surface = Cairo::ImageSurface::create(Cairo::ImageSurface::Format::ARGB32, width, height);
        auto cr      = Cairo::Context::create(surface);

        pdf::Renderer renderer(*page, cr);
        renderer.render();

        const std::string filename = "test-" + std::to_string(i) + ".png";
        surface->write_to_png(filename);
    }
}
#endif
