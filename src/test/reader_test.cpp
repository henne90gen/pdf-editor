#include <gtest/gtest.h>

#include <pdf/document.h>

TEST(Reader, Blank) {
    pdf::Document document;
    pdf::Document::load_from_file("../../../test-files/blank.pdf", document);
    std::vector<pdf::IndirectObject *> objects = document.getAllObjects();
    ASSERT_EQ(objects.size(), 8);

    auto root = document.root();
    ASSERT_NE(root, nullptr);

    auto pages = document.pages();
    ASSERT_EQ(pages.size(), 1);

    auto page        = pages[0];
    auto contentsOpt = page->contents();
    ASSERT_TRUE(contentsOpt.has_value());
    auto contents = contentsOpt.value();
    ASSERT_TRUE(contents->is<pdf::Stream>());
    auto stream  = contents->as<pdf::Stream>();
    auto str     = stream->to_string();
    auto filters = stream->filters();
}

TEST(Reader, HelloWorld) {
    pdf::Document document;
    pdf::Document::load_from_file("../../../test-files/hello-world.pdf", document);
    std::vector<pdf::IndirectObject *> objects = document.getAllObjects();
    ASSERT_EQ(objects.size(), 13);

    auto root = document.root();
    ASSERT_NE(root, nullptr);

    auto pages = document.pages();
    ASSERT_EQ(pages.size(), 1);

    auto page        = pages[0];
    auto contentsOpt = page->contents();
    ASSERT_TRUE(contentsOpt.has_value());
    auto contents = contentsOpt.value();
    ASSERT_TRUE(contents->is<pdf::Stream>());
    auto stream  = contents->as<pdf::Stream>();
    auto str     = stream->to_string();
    auto filters = stream->filters();
}
