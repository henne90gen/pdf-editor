#include <gtest/gtest.h>

#include <fstream>
#include <pdf/document.h>
#include <pdf/image.h>

void assertFilesAreIdentical(const std::string &f1, const std::string &f2) {
    std::ifstream ifs1(f1, std::ios::in | std::ios::ate | std::ios::binary);
    std::ifstream ifs2(f2, std::ios::in | std::ios::ate | std::ios::binary);

    auto f1Size = ifs1.tellg();
    ifs1.seekg(0);
    auto f2Size = ifs2.tellg();
    ifs2.seekg(0);
    ASSERT_EQ(f1Size, f2Size);

    int count = 0;
    while (true) {
        char c1, c2;
        ifs1.read(&c1, 1);
        ifs2.read(&c2, 1);
        if (ifs1.eof() || ifs2.eof()) {
            break;
        }

        if (count < 54) {
            count++;
            continue;
        }

        ASSERT_EQ(c1, c2) << "Byte " << count;

        count++;
    }
}

void assertImageCanBeExtracted(pdf::Image &image, const std::string &readFileName, const std::string &writeFileName,
                               int32_t width, int32_t height, uint16_t bitsPerComponent) {
    ASSERT_EQ(width, image.width);
    ASSERT_EQ(height, image.height);
    ASSERT_EQ(bitsPerComponent, image.bitsPerComponent);

    auto writeResult = image.write_bmp(writeFileName);
    ASSERT_FALSE(writeResult.has_error());

    auto finalFileName = readFileName.substr(0, readFileName.size() - 4) + ".bmp";
    assertFilesAreIdentical(finalFileName, writeFileName);
}

TEST(Image, ExtractImage1) {
    auto readFileName  = "../../../test-files/image-1.pdf";
    auto writeFileName = "image-1.bmp";

    auto result = pdf::Document::read_from_file(readFileName, false);
    ASSERT_FALSE(result.has_error());
    auto document = result.value();
    document.for_each_image([&readFileName, &writeFileName](pdf::Image &image) {
        assertImageCanBeExtracted(image, readFileName, writeFileName, 100, 100, 8);
        return pdf::ForEachResult::CONTINUE;
    });
}

TEST(Image, ExtractImage2) {
    auto readFileName  = "../../../test-files/image-2.pdf";
    auto writeFileName = "image-2.bmp";

    auto result = pdf::Document::read_from_file(readFileName, false);
    ASSERT_FALSE(result.has_error());
    auto document = result.value();
    document.for_each_image([&readFileName, &writeFileName](pdf::Image &image) {
        assertImageCanBeExtracted(image, readFileName, writeFileName, 4, 4, 8);
        return pdf::ForEachResult::CONTINUE;
    });
}

TEST(Image, ExtractImage3) {
    auto readFileName  = "../../../test-files/image-3.pdf";
    auto writeFileName = "image-3.bmp";

    auto result = pdf::Document::read_from_file(readFileName, false);
    ASSERT_FALSE(result.has_error());
    auto document = result.value();
    document.for_each_image([&readFileName, &writeFileName](pdf::Image &image) {
        assertImageCanBeExtracted(image, readFileName, writeFileName, 2, 2, 8);
        return pdf::ForEachResult::CONTINUE;
    });
}
