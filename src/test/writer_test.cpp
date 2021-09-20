#include <filesystem>
#include <gtest/gtest.h>
#include <iostream>

#include <pdf/document.h>

TEST(Writer, Blank) {
    pdf::Document document;
    pdf::Document::load_from_file("../../../test-files/blank.pdf", document);
    std::string filePath = "blank.pdf";
    auto error           = document.save_to_file(filePath);
    ASSERT_FALSE(error);
    ASSERT_TRUE(std::filesystem::exists(std::filesystem::path(filePath)));
}

TEST(Writer, HelloWorld) {
    pdf::Document document;
    pdf::Document::load_from_file("../../../test-files/hello-world.pdf", document);
    std::string filePath = "hello-world.pdf";
    auto error           = document.save_to_file(filePath);
    ASSERT_FALSE(error);
    ASSERT_TRUE(std::filesystem::exists(std::filesystem::path(filePath)));
}

TEST(Writer, DeletePageInvalidPageNum) {
    pdf::Document document;
    pdf::Document::load_from_file("../../../test-files/two-pages.pdf", document);
    ASSERT_TRUE(document.delete_page(0));
    ASSERT_TRUE(document.delete_page(-1));
    ASSERT_TRUE(document.delete_page(3));
}

TEST(Writer, DeletePageFirst) {
    pdf::Document document;
    pdf::Document::load_from_file("../../../test-files/two-pages.pdf", document);
    ASSERT_FALSE(document.delete_page(1));
    ASSERT_EQ(document.page_count(), 1);
    std::string filePath = "delete-page-first.pdf";
    auto result          = document.save_to_file(filePath);
    ASSERT_FALSE(result);
    ASSERT_TRUE(std::filesystem::exists(std::filesystem::path(filePath)));
}

TEST(Writer, DeletePageSecond) {
    pdf::Document document;
    pdf::Document::load_from_file("../../../test-files/two-pages.pdf", document);
    ASSERT_FALSE(document.delete_page(2));
    ASSERT_EQ(document.page_count(), 1);

    char *buffer = nullptr;
    size_t size  = 0;
    ASSERT_FALSE(document.save_to_memory(buffer, size));
    ASSERT_NE(buffer, nullptr);
    ASSERT_NE(size, 0);

    pdf::Document testDoc;
    ASSERT_FALSE(pdf::Document::load_from_memory(buffer, size, testDoc));
    ASSERT_EQ(testDoc.page_count(), 1);
}
