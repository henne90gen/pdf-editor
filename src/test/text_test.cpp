#include <gtest/gtest.h>

#include <pdf/document.h>
#include <pdf/page.h>

TEST(Text, HelloWorldTextBlocks) {
    auto document_result_1 = pdf::Document::read_from_file("../../../test-files/hello-world.pdf");
    ASSERT_FALSE(document_result_1.has_error());

    auto &document = document_result_1.value();
    auto pages    = document.pages();
    ASSERT_EQ(pages.size(), 1);

    auto page       = pages[0];
    auto textBlocks = page->text_blocks();
    ASSERT_EQ(textBlocks.size(), 1);

    ASSERT_EQ(textBlocks[0].text, "Hello World");
    ASSERT_DOUBLE_EQ(textBlocks[0].x, 56.8);
    ASSERT_DOUBLE_EQ(textBlocks[0].y, 773.98900000000003);
    ASSERT_DOUBLE_EQ(textBlocks[0].width, 58.902000000000001);
    ASSERT_DOUBLE_EQ(textBlocks[0].height, 9);
}

TEST(Text, Move) {
    auto document_result_1 = pdf::Document::read_from_file("../../../test-files/hello-world.pdf");
    ASSERT_FALSE(document_result_1.has_error());

    auto &document = document_result_1.value();
    {
        auto pages = document.pages();
        ASSERT_EQ(pages.size(), 1);

        auto page       = pages[0];
        auto textBlocks = page->text_blocks();
        ASSERT_EQ(textBlocks.size(), 1);

        textBlocks[0].move(document, 10, 500);
    }

    uint8_t *buffer = nullptr;
    size_t size     = 0;
    ASSERT_FALSE(document.write_to_memory(buffer, size).has_error());

    auto document_result_2 = pdf::Document::read_from_memory(buffer, size);
    ASSERT_FALSE(document_result_2.has_error());

    auto &newDocument = document_result_2.value();
    {
        auto pages = newDocument.pages();
        ASSERT_EQ(pages.size(), 1);

        auto page       = pages[0];
        auto textBlocks = page->text_blocks();
        ASSERT_EQ(textBlocks.size(), 1);

        ASSERT_EQ(textBlocks[0].text, "Hello World");
        ASSERT_DOUBLE_EQ(textBlocks[0].x, 66.8);
        ASSERT_DOUBLE_EQ(textBlocks[0].y, 273.98900000000003);
        ASSERT_DOUBLE_EQ(textBlocks[0].width, 58.902000000000001);
        ASSERT_DOUBLE_EQ(textBlocks[0].height, 9);
    }
}
