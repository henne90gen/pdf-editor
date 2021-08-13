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
