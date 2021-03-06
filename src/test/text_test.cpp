#include <gtest/gtest.h>

#include <pdf/document.h>
#include <pdf/page.h>

#include "test_util.h"

TEST(Text, HelloWorldTextBlocks) {
    pdf::Document document;
    pdf::Document::read_from_file("../../../test-files/hello-world.pdf", document);

    auto pages = document.pages();
    ASSERT_EQ(pages.size(), 1);

    auto page       = pages[0];
    auto textBlocks = page->text_blocks();
    ASSERT_EQ(textBlocks.size(), 1);

    ASSERT_EQ(textBlocks[0].text, "Hello World");
    ASSERT_DOUBLE_EQ(textBlocks[0].x, 56.8);
    ASSERT_DOUBLE_EQ(textBlocks[0].y, 67.900763779528006);
    ASSERT_DOUBLE_EQ(textBlocks[0].width, 58.902000000000001);
    ASSERT_DOUBLE_EQ(textBlocks[0].height, 9);
}

TEST(Text, Move) {
    pdf::Document document;
    pdf::Document::read_from_file("../../../test-files/hello-world.pdf", document);

    {
        auto pages = document.pages();
        ASSERT_EQ(pages.size(), 1);

        auto page       = pages[0];
        auto textBlocks = page->text_blocks();
        ASSERT_EQ(textBlocks.size(), 1);

        textBlocks[0].move(document, 10, 500);
    }

    char *buffer = nullptr;
    size_t size  = 0;
    ASSERT_FALSE(document.write_to_memory(buffer, size).has_error());

    pdf::Document newDocument;
    ASSERT_FALSE(pdf::Document::read_from_memory(buffer, size, newDocument).has_error());

    {
        auto pages = newDocument.pages();
        ASSERT_EQ(pages.size(), 1);

        auto page       = pages[0];
        auto textBlocks = page->text_blocks();
        ASSERT_EQ(textBlocks.size(), 1);

        ASSERT_EQ(textBlocks[0].text, "Hello World");
        ASSERT_DOUBLE_EQ(textBlocks[0].x, 66.8);
        ASSERT_DOUBLE_EQ(textBlocks[0].y, 567.90076399999998);
        ASSERT_DOUBLE_EQ(textBlocks[0].width, 58.902000000000001);
        ASSERT_DOUBLE_EQ(textBlocks[0].height, 9);
    }
}
