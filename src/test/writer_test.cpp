#include <filesystem>
#include <gtest/gtest.h>
#include <iostream>

#include <pdf/document.h>

#include "test_util.h"

TEST(Writer, Blank) {
    pdf::Document document;
    pdf::Document::read_from_file("../../../test-files/blank.pdf", document);
    std::string filePath = "blank.pdf";
    auto error           = document.write_to_file(filePath);
    ASSERT_FALSE(error);
    ASSERT_TRUE(std::filesystem::exists(std::filesystem::path(filePath)));
}

TEST(Writer, HelloWorld) {
    pdf::Document document;
    pdf::Document::read_from_file("../../../test-files/hello-world.pdf", document);
    std::string filePath = "hello-world.pdf";
    auto error           = document.write_to_file(filePath);
    ASSERT_FALSE(error);
    ASSERT_TRUE(std::filesystem::exists(std::filesystem::path(filePath)));
}

TEST(Writer, DeletePageInvalidPageNum) {
    pdf::Document document;
    pdf::Document::read_from_file("../../../test-files/two-pages.pdf", document);
    ASSERT_TRUE(document.delete_page(0));
    ASSERT_TRUE(document.delete_page(-1));
    ASSERT_TRUE(document.delete_page(3));
}

TEST(Writer, DeletePageFirst) {
    pdf::Document document;
    pdf::Document::read_from_file("../../../test-files/two-pages.pdf", document);
    ASSERT_FALSE(document.delete_page(1));
    ASSERT_EQ(document.page_count(), 1);
    std::string filePath = "delete-page-first.pdf";
    auto result          = document.write_to_file(filePath);
    ASSERT_FALSE(result);
    ASSERT_TRUE(std::filesystem::exists(std::filesystem::path(filePath)));
}

TEST(Writer, DeletePageSecond) {
    pdf::Document document;
    pdf::Document::read_from_file("../../../test-files/two-pages.pdf", document);
    ASSERT_FALSE(document.delete_page(2));
    ASSERT_EQ(document.page_count(), 1);

    char *buffer = nullptr;
    size_t size  = 0;
    ASSERT_FALSE(document.write_to_memory(buffer, size));
    ASSERT_NE(buffer, nullptr);
    ASSERT_NE(size, 0);

    pdf::Document testDoc;
    ASSERT_FALSE(pdf::Document::read_from_memory(buffer, size, testDoc));
    ASSERT_EQ(testDoc.page_count(), 1);
    free(buffer);
}

TEST(Writer, Embed) {
    pdf::Document document;
    pdf::Document::read_from_file("../../../test-files/hello-world.pdf", document);
    ASSERT_FALSE(document.embed_file("../../../test-files/hello-world.pdf"));

    char *buffer = nullptr;
    size_t size  = 0;
    ASSERT_FALSE(document.write_to_memory(buffer, size));
    ASSERT_NE(buffer, nullptr);
    ASSERT_NE(size, 0);

    ASSERT_BUFFER_CONTAINS_AT(
          buffer, "14 0 obj <<\n/Length 6650\n/Filter /FlateDecode\n/FileMetadata << /Name (hello-world.pdf) /Executable false >>\n>> stream\n",
          6867);
    ASSERT_BUFFER_CONTAINS_AT(buffer, "endstream endobj\n", 13635);
    ASSERT_BUFFER_CONTAINS_AT(buffer, "xref\n0 15\n", 13652);
    ASSERT_BUFFER_CONTAINS_AT(buffer, "0000006692 00000 n", 13922);
}
