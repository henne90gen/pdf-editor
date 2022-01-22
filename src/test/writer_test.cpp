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
    ASSERT_FALSE(error.has_error());
    ASSERT_TRUE(std::filesystem::exists(std::filesystem::path(filePath)));
}

TEST(Writer, HelloWorld) {
    pdf::Document document;
    pdf::Document::read_from_file("../../../test-files/hello-world.pdf", document);
    std::string filePath = "hello-world.pdf";
    auto error           = document.write_to_file(filePath);
    ASSERT_FALSE(error.has_error());
    ASSERT_TRUE(std::filesystem::exists(std::filesystem::path(filePath)));
}

TEST(Writer, DeletePageInvalidPageNum) {
    pdf::Document document;
    pdf::Document::read_from_file("../../../test-files/two-pages.pdf", document);
    ASSERT_TRUE(document.delete_page(0).has_error());
    ASSERT_TRUE(document.delete_page(-1).has_error());
    ASSERT_TRUE(document.delete_page(3).has_error());
}

TEST(Writer, DeletePageFirst) {
    pdf::Document document;
    pdf::Document::read_from_file("../../../test-files/two-pages.pdf", document);
    auto result = document.delete_page(1);
    ASSERT_FALSE(result.has_error());
    ASSERT_EQ(document.page_count(), 1);
    std::string filePath = "delete-page-first.pdf";
    result               = document.write_to_file(filePath);
    ASSERT_FALSE(result.has_error());
    ASSERT_TRUE(std::filesystem::exists(std::filesystem::path(filePath)));
}

TEST(Writer, DISABLED_DeletePageSecond) {
    pdf::Document document;
    pdf::Document::read_from_file("../../../test-files/two-pages.pdf", document);
    auto result = document.delete_page(2);
    ASSERT_FALSE(result.has_error());
    ASSERT_EQ(document.page_count(), 1);

    char *buffer = nullptr;
    size_t size  = 0;
    ASSERT_FALSE(document.write_to_memory(buffer, size).has_error());
    ASSERT_NE(buffer, nullptr);
    ASSERT_NE(size, 0);

    pdf::Document testDoc;
    ASSERT_FALSE(pdf::Document::read_from_memory(buffer, size, testDoc).has_error());
    ASSERT_EQ(testDoc.page_count(), 1);
    free(buffer);
}

TEST(Writer, DISABLED_Embed) {
    pdf::Document document;
    pdf::Document::read_from_file("../../../test-files/hello-world.pdf", document);
    ASSERT_FALSE(document.embed_file("../../../test-files/hello-world.pdf").has_error());

    char *buffer = nullptr;
    size_t size  = 0;
    ASSERT_FALSE(document.write_to_memory(buffer, size).has_error());
    ASSERT_NE(buffer, nullptr);
    ASSERT_NE(size, 0);

    ASSERT_BUFFER_CONTAINS_AT(buffer, 6867,
                              "14 0 obj <<\n/Length 6650\n/Filter /FlateDecode\n/FileMetadata << /Name "
                              "(hello-world.pdf) /Executable false >>\n>> stream\n");
    ASSERT_BUFFER_CONTAINS_AT(buffer, 13635, "endstream endobj\n");
    ASSERT_BUFFER_CONTAINS_AT(buffer, 13652, "xref\n0 15\n");
    ASSERT_BUFFER_CONTAINS_AT(buffer, 13922, "0000006692 00000 n");
}

TEST(Writer, DISABLED_AddRawSection) {
    pdf::Document document;
    pdf::Document::read_from_file("../../../test-files/hello-world.pdf", document);

    //    char buf[7] = "/Hello";
    //    document.add_raw_section(document.file.data + 6059, buf, 6);

    char *buffer = nullptr;
    size_t size  = 0;
    ASSERT_FALSE(document.write_to_memory(buffer, size).has_error());
    ASSERT_NE(buffer, nullptr);
    ASSERT_NE(size, 0);

    ASSERT_BUFFER_CONTAINS_AT(buffer, 6057, "<</Hello/Type");
    ASSERT_BUFFER_CONTAINS_AT(buffer, 6903, "0000006333");
    ASSERT_BUFFER_CONTAINS_AT(buffer, 6963, "0000006502");
    ASSERT_BUFFER_CONTAINS_AT(buffer, 7083, "0000006246");
    ASSERT_BUFFER_CONTAINS_AT(buffer, 7123, "0000006601");
    ASSERT_BUFFER_CONTAINS_AT(buffer, 7143, "0000006698");
    ASSERT_BUFFER_CONTAINS_AT(buffer, 7345, "6873");
}

TEST(Writer, Header) {
    pdf::Document document;
    pdf::Document::read_from_file("../../../test-files/blank.pdf", document);
    char *buffer = nullptr;
    size_t size  = 0;
    auto error   = document.write_to_memory(buffer, size);
    ASSERT_FALSE(error.has_error());
    // %PDF-1.6\n%äüöß\n
    ASSERT_BUFFER_CONTAINS_AT(buffer, 0, "%PDF-1.6\n%\xC3\xA4\xC3\xBC\xC3\xB6\xC3\x9F\n");
}

TEST(Writer, Objects) {
    pdf::Document document;
    pdf::Document::read_from_file("../../../test-files/blank.pdf", document);
    char *buffer = nullptr;
    size_t size  = 0;
    auto error   = document.write_to_memory(buffer, size);
    ASSERT_FALSE(error.has_error());
    ASSERT_BUFFER_CONTAINS_AT(buffer, 19, "8 0 obj");
}

TEST(Writer, Trailer) {
    pdf::Document document;
    pdf::Document::read_from_file("../../../test-files/blank.pdf", document);
    char *buffer = nullptr;
    size_t size  = 0;
    auto anError = document.write_to_file("test.pdf");
    ASSERT_FALSE(anError.has_error());
    auto error = document.write_to_memory(buffer, size);
    ASSERT_FALSE(error.has_error());
    ASSERT_BUFFER_CONTAINS_AT(buffer, 754, "\n\nxref 0 8");
    ASSERT_BUFFER_CONTAINS_AT(buffer, 764, "0000000000 65535 f \n");
    ASSERT_BUFFER_CONTAINS_AT(buffer, 784, "0000000588 00000 n \n");
    ASSERT_BUFFER_CONTAINS_AT(buffer, 804, "0000000469 00000 n \n");
    ASSERT_BUFFER_CONTAINS_AT(buffer, 824, "0000000452 00000 n \n");
    ASSERT_BUFFER_CONTAINS_AT(buffer, 844, "0000000356 00000 n \n");
    ASSERT_BUFFER_CONTAINS_AT(buffer, 864, "0000000336 00000 n \n");
    ASSERT_BUFFER_CONTAINS_AT(buffer, 884, "0000000285 00000 n \n");
    ASSERT_BUFFER_CONTAINS_AT(buffer, 904, "0000000191 00000 n \n");
    ASSERT_BUFFER_CONTAINS_AT(buffer, 924, "0000000019 00000 n \n");
    ASSERT_BUFFER_CONTAINS_AT(buffer, 944, "trailer\n<</Size 9");
    ASSERT_BUFFER_CONTAINS_AT(buffer, 1110, ">>\nstartxref\n754\n%%EOF\n");
}
