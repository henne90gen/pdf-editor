#include <gtest/gtest.h>

#include <pdf_reader.h>

TEST(Reader, Blank) {
    pdf::File file;
    pdf::load_from_file("../../../test-files/blank.pdf", file);
    std::vector<pdf::IndirectObject *> objects = file.getAllObjects();
    ASSERT_EQ(objects.size(), 8);

    auto root = file.getRoot();
    ASSERT_NE(root, nullptr);

    auto pageTree = file.getPageTree();
    ASSERT_NE(pageTree, nullptr);

    auto pages = pageTree->pages(file);
    ASSERT_EQ(pages.size(), 1);

    auto page        = pages[0];
    auto contentsOpt = page->contents(file);
    ASSERT_TRUE(contentsOpt.has_value());
    auto contents    = contentsOpt.value();
    auto contentsObj = file.resolve(contents->as<pdf::IndirectReference>());
    ASSERT_TRUE(contentsObj->object->is<pdf::Stream>());
    auto stream  = contentsObj->object->as<pdf::Stream>();
    auto str     = stream->to_string();
    auto filters = stream->filters();
    std::cout << "";
}

TEST(Reader, HelloWorld) {
    pdf::File file;
    pdf::load_from_file("../../../test-files/hello-world.pdf", file);
    std::vector<pdf::IndirectObject *> objects = file.getAllObjects();
    ASSERT_EQ(objects.size(), 13);

    auto root = file.getRoot();
    ASSERT_NE(root, nullptr);

    auto pageTree = file.getPageTree();
    ASSERT_NE(pageTree, nullptr);

    auto pages = pageTree->pages(file);
    ASSERT_EQ(pages.size(), 1);

    auto page        = pages[0];
    auto contentsOpt = page->contents(file);
    ASSERT_TRUE(contentsOpt.has_value());
    auto contents    = contentsOpt.value();
    auto contentsObj = file.resolve(contents->as<pdf::IndirectReference>());
    ASSERT_TRUE(contentsObj->object->is<pdf::Stream>());
    auto stream  = contentsObj->object->as<pdf::Stream>();
    auto str     = stream->to_string();
    auto filters = stream->filters();
}
