#include <gtest/gtest.h>

#include <pdf/document.h>
#include <pdf/renderer.h>

TEST(Renderer, Blank) {
    pdf::Document document;
    pdf::Document::load_from_file("../../../test-files/blank.pdf", document);

    auto pages = document.pages();
    for (auto page : pages) {
        pdf::renderer renderer;
        renderer.render(page);
    }
}

TEST(Renderer, HelloWorld) {
    pdf::Document document;
    pdf::Document::load_from_file("../../../test-files/hello-world.pdf", document);

    auto pages = document.pages();
    for (auto page : pages) {
        pdf::renderer renderer;
        renderer.render(page);
    }
}
