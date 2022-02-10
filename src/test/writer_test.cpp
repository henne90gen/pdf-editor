#include <filesystem>
#include <gtest/gtest.h>
#include <iostream>

#include <pdf/document.h>

#include "../util/process.h"
#include "test_util.h"

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

std::vector<std::string_view> split_by_lines(const std::string &input) {
    size_t lastStartIndex = 0;
    size_t currentIndex   = 0;
    auto result           = std::vector<std::string_view>();
    while (currentIndex < input.size()) {
        if (input[currentIndex] == '\n') {
            result.emplace_back(input.data() + lastStartIndex, currentIndex - lastStartIndex);
            lastStartIndex = currentIndex + 1;
        }
        currentIndex++;
    }
    result.emplace_back(input.data() + lastStartIndex, currentIndex - lastStartIndex);
    return result;
}

TEST(SplitByLines, Simple) {
    auto input = "abc\ndef\nghi";
    auto lines = split_by_lines(input);
    ASSERT_EQ(3, lines.size());
    ASSERT_EQ("abc", lines[0]);
    ASSERT_EQ("def", lines[1]);
    ASSERT_EQ("ghi", lines[2]);
}
TEST(SplitByLines, EndsWithNewline) {
    auto input = "abc\ndef\nghi\n";
    auto lines = split_by_lines(input);
    ASSERT_EQ(4, lines.size());
    ASSERT_EQ("abc", lines[0]);
    ASSERT_EQ("def", lines[1]);
    ASSERT_EQ("ghi", lines[2]);
    ASSERT_EQ("", lines[3]);
}

void write_pdf(const std::string &name) {
    pdf::Document document;
    std::string testFilePath = "../../../test-files/" + name;
    pdf::Document::read_from_file(testFilePath, document);

    auto anError = document.write_to_file(name);
    ASSERT_FALSE(anError.has_error());

    auto expectedResult = process::execute("/usr/bin/pdfinfo", {testFilePath});
    auto actualResult   = process::execute("/usr/bin/pdfinfo", {name});
    ASSERT_EQ(expectedResult.status, actualResult.status);
    ASSERT_EQ(expectedResult.error, actualResult.error);
    auto expectedOutputLines = split_by_lines(expectedResult.output);
    auto actualOutputLines   = split_by_lines(actualResult.output);
    for (size_t i = 0; i < expectedOutputLines.size(); i++) {
        if (expectedOutputLines[i].starts_with("File size:")) {
            continue;
        }
        ASSERT_EQ(expectedOutputLines[i], actualOutputLines[i]);
    }
}

TEST(Writer, Blank) { write_pdf("blank.pdf"); }
TEST(Writer, HelloWorld) { write_pdf("hello-world.pdf"); }
TEST(Writer, Image1) { write_pdf("image-1.pdf"); }
TEST(Writer, Image2) { write_pdf("image-2.pdf"); }
TEST(Writer, DISABLED_ObjectStream) {
    // TODO
    // All objects (even the ones that are part of an object stream) are loaded into the objectList
    // They are then all written out into the final pdf file. This is probably problematic.
    write_pdf("object-stream.pdf");
}
TEST(Writer, TwoPages) { write_pdf("two-pages.pdf"); }
