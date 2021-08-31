#include <filesystem>
#include <gtest/gtest.h>

#include <pdf/document.h>

TEST(Writer, Blank) {
    pdf::Document document;
    pdf::Document::loadFromFile("../../../test-files/blank.pdf", document);
    std::string filePath = "blank.pdf";
    auto result          = document.saveToFile(filePath);
    ASSERT_TRUE(result);
    ASSERT_TRUE(std::filesystem::exists(std::filesystem::path(filePath)));
}

TEST(Writer, HelloWorld) {
    pdf::Document document;
    pdf::Document::loadFromFile("../../../test-files/hello-world.pdf", document);
    std::string filePath = "hello-world.pdf";
    auto result          = document.saveToFile(filePath);
    ASSERT_TRUE(result);
    ASSERT_TRUE(std::filesystem::exists(std::filesystem::path(filePath)));
}

TEST(Writer, DeletePageInvalidPageNum) {
    pdf::Document document;
    pdf::Document::loadFromFile("../../../test-files/two-pages.pdf", document);
    ASSERT_FALSE(document.delete_page(0));
    ASSERT_FALSE(document.delete_page(-1));
    ASSERT_FALSE(document.delete_page(3));
}

TEST(Writer, DeletePageFirst) {
    pdf::Document document;
    pdf::Document::loadFromFile("../../../test-files/two-pages.pdf", document);
    ASSERT_TRUE(document.delete_page(1));
    ASSERT_EQ(document.page_count(), 1);
    std::string filePath = "delete-page-first.pdf";
    auto result          = document.saveToFile(filePath);
    ASSERT_TRUE(result);
    ASSERT_TRUE(std::filesystem::exists(std::filesystem::path(filePath)));
}

TEST(Writer, DeletePageSecond) {
    pdf::Document document;
    pdf::Document::loadFromFile("../../../test-files/two-pages.pdf", document);
    ASSERT_TRUE(document.delete_page(2));
    ASSERT_EQ(document.page_count(), 1);
    std::string filePath = "delete-page-second.pdf";
    auto result          = document.saveToFile(filePath);
    ASSERT_TRUE(result);
    ASSERT_TRUE(std::filesystem::exists(std::filesystem::path(filePath)));
}
