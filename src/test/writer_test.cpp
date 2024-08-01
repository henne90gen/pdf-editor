#include <filesystem>
#include <gtest/gtest.h>
#include <iostream>
#include <pdf/document.h>
#include <pdf/page.h>
#include <sstream>

#include "process.h"
#include "test_util.h"

TEST(Writer, bla) {
    auto s     = std::stringstream();
    uint32_t i = 1230;
    s << i;
    ASSERT_EQ("1230", s.str());
}

TEST(Writer, DeletePageInvalidPageNum) {
    auto result = pdf::Document::read_from_file("../../../test-files/two-pages.pdf");
    ASSERT_FALSE(result.has_error()) << result.message();

    auto &document = result.value();
    ASSERT_TRUE(document.delete_page(0).has_error());
    ASSERT_TRUE(document.delete_page(-1).has_error());
    ASSERT_TRUE(document.delete_page(3).has_error());
}

TEST(Writer, DeletePageFirst) {
    auto documentResult = pdf::Document::read_from_file("../../../test-files/two-pages.pdf");
    ASSERT_FALSE(documentResult.has_error()) << documentResult.message();

    auto &document = documentResult.value();
    auto result    = document.delete_page(1);
    ASSERT_FALSE(result.has_error());
    ASSERT_EQ(document.page_count(), 1);
    std::string filePath = "delete-page-first.pdf";
    result               = document.write_to_file(filePath);
    ASSERT_FALSE(result.has_error());
    ASSERT_TRUE(std::filesystem::exists(std::filesystem::path(filePath)));
}

TEST(Writer, DISABLED_DeletePageSecond) {
    auto documentResult = pdf::Document::read_from_file("../../../test-files/two-pages.pdf");
    ASSERT_FALSE(documentResult.has_error()) << documentResult.message();

    auto &document = documentResult.value();
    auto result    = document.delete_page(2);
    ASSERT_FALSE(result.has_error());
    ASSERT_EQ(document.page_count(), 1);

    uint8_t *buffer = nullptr;
    size_t size     = 0;
    ASSERT_FALSE(document.write_to_memory(buffer, size).has_error());
    ASSERT_NE(buffer, nullptr);
    ASSERT_NE(size, 0);

    auto testDocResult = pdf::Document::read_from_memory(buffer, size);
    ASSERT_FALSE(testDocResult.has_error()) << testDocResult.message();

    auto &testDoc = testDocResult.value();
    ASSERT_EQ(testDoc.page_count(), 1);
    free(buffer);
}

TEST(Writer, MoveImage) {
    auto result = pdf::Document::read_from_file("../../../test-files/image-1.pdf");
    ASSERT_FALSE(result.has_error()) << result.message();
    auto &document = result.value();
    document.for_each_page([&document](pdf::Page *page) {
        page->for_each_image([&document](pdf::PageImage &image) {
            image.move(document, 10, 10);
            return pdf::ForEachResult::CONTINUE;
        });
        return pdf::ForEachResult::CONTINUE;
    });

    uint8_t *buffer = nullptr;
    size_t size     = 0;
    ASSERT_FALSE(document.write_to_memory(buffer, size).has_error());
    ASSERT_NE(buffer, nullptr);
    ASSERT_NE(size, 0);

    auto assertFunc = [](pdf::PageImage &image) {
        ASSERT_EQ(95.5, image.xOffset);
        ASSERT_EQ(704.189, image.yOffset);
    };

    auto docResult = pdf::Document::read_from_memory(buffer, size);
    ASSERT_FALSE(docResult.has_error());
    auto &doc = docResult.value();
    doc.for_each_page([&assertFunc](pdf::Page *page) {
        page->for_each_image([&assertFunc](pdf::PageImage &image) {
            assertFunc(image);
            return pdf::ForEachResult::CONTINUE;
        });
        return pdf::ForEachResult::CONTINUE;
    });
}

TEST(Writer, Embed) {
    auto result_1 = pdf::Document::read_from_file("../../../test-files/hello-world.pdf");
    ASSERT_FALSE(result_1.has_error()) << result_1.message();
    auto &document = result_1.value();

    ASSERT_FALSE(document.embed_file("../../../test-files/hello-world.pdf").has_error());

    //    ASSERT_FALSE(document.write_to_file("test.pdf").has_error());

    uint8_t *buffer = nullptr;
    size_t size     = 0;
    ASSERT_FALSE(document.write_to_memory(buffer, size).has_error());
    ASSERT_NE(buffer, nullptr);
    ASSERT_NE(size, 0);

    auto assertFunc = [](pdf::EmbeddedFile *embeddedFile) {
        ASSERT_EQ("EmbeddedFile", embeddedFile->dictionary->must_find<pdf::Name>("Type")->value);
        ASSERT_EQ(6650, embeddedFile->dictionary->must_find<pdf::Integer>("Length")->value);
        auto params = embeddedFile->dictionary->must_find<pdf::Dictionary>("Params");
        ASSERT_EQ(7350, params->must_find<pdf::Integer>("Size")->value);
    };

    auto result_2 = pdf::Document::read_from_memory(buffer, size);
    ASSERT_FALSE(result_2.has_error()) << result_2.message();
    auto &doc = result_2.value();
    doc.for_each_embedded_file([&assertFunc](pdf::EmbeddedFile *embeddedFile) {
        assertFunc(embeddedFile);
        return pdf::ForEachResult::CONTINUE;
    });
}

TEST(Writer, Header) {
    auto result = pdf::Document::read_from_file("../../../test-files/blank.pdf");
    ASSERT_FALSE(result.has_error()) << result.message();

    auto &document  = result.value();
    uint8_t *buffer = nullptr;
    size_t size     = 0;
    auto error      = document.write_to_memory(buffer, size);
    ASSERT_FALSE(error.has_error());
    // %PDF-1.6\n%äüöß\n
    ASSERT_BUFFER_CONTAINS_AT(buffer, 0, "%PDF-1.6\n%\xC3\xA4\xC3\xBC\xC3\xB6\xC3\x9F\n");
}

TEST(Writer, Objects) {
    auto result = pdf::Document::read_from_file("../../../test-files/blank.pdf");
    ASSERT_FALSE(result.has_error()) << result.message();
    auto &document  = result.value();
    uint8_t *buffer = nullptr;
    size_t size     = 0;
    auto error      = document.write_to_memory(buffer, size);
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
    std::string testFilePath = "../../../test-files/" + name;
    auto result              = pdf::Document::read_from_file(testFilePath);
    ASSERT_FALSE(result.has_error()) << result.message();

    auto &document = result.value();
    auto anError   = document.write_to_file(name);
    ASSERT_FALSE(anError.has_error()) << anError.message();

#if _WIN32
    const char *command = "C:/Program Files/Xpdf/bin64/pdfinfo.exe";
#else
    const char *command = "/usr/bin/pdfinfo";
#endif
    auto expectedResult = pdf::execute(command, {testFilePath});
    ASSERT_FALSE(expectedResult.has_error()) << expectedResult.message();

    auto actualResult = pdf::execute(command, {name});
    ASSERT_FALSE(actualResult.has_error()) << actualResult.message();
    ASSERT_EQ(expectedResult.value().status, actualResult.value().status);
    ASSERT_EQ(expectedResult.value().error, actualResult.value().error);

    auto expectedOutputLines = split_by_lines(expectedResult.value().output);
    auto actualOutputLines   = split_by_lines(actualResult.value().output);
    for (size_t i = 0; i < expectedOutputLines.size(); i++) {
        if (expectedOutputLines[i].starts_with("File size:")) {
            // ignore line with "File size:"
            continue;
        }
        ASSERT_EQ(expectedOutputLines[i], actualOutputLines[i]);
    }
}

TEST(Writer, Blank) { write_pdf("blank.pdf"); }
TEST(Writer, HelloWorld) { write_pdf("hello-world.pdf"); }
TEST(Writer, Image1) { write_pdf("image-1.pdf"); }
TEST(Writer, Image2) { write_pdf("image-2.pdf"); }
TEST(Writer, Image3) { write_pdf("image-3.pdf"); }
TEST(Writer, DISABLED_ObjectStream) {
    // TODO
    // All objects (even the ones that are part of an object stream) are loaded into the objectList
    // They are then all written out into the final pdf file. This is problematic.
    write_pdf("object-stream.pdf");
}
TEST(Writer, TwoPages) { write_pdf("two-pages.pdf"); }
