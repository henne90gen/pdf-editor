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

    char buf1[1024];
    char buf2[1024];
    int count = 0;
    while (true) {
        ifs1.read(buf1, 1024);
        ifs2.read(buf2, 1024);
        for (int i = 0; i < 1024; i++) {
            auto currentByte = i + count * 1024;
            if (currentByte < 54) {
                continue;
            }
            if (currentByte > f1Size || currentByte > f2Size) {
                break;
            }
            ASSERT_EQ(buf1[i], buf2[i]) << "Byte " << currentByte;
        }
        if (ifs1.eof() || ifs2.eof()) {
            break;
        }
        count++;
    }
}

void assertImageCanBeExtracted(pdf::Image &image, const std::string &readFileName) {
    ASSERT_EQ(2, image.width);
    ASSERT_EQ(2, image.height);
    ASSERT_EQ(8, image.bitsPerComponent);

    const char *writeFileName = "image-3.bmp";
    auto writeResult          = image.write_bmp(writeFileName);
    ASSERT_FALSE(writeResult.has_error());
    auto finalFileName = readFileName.substr(0, readFileName.size() - 4) + ".bmp";
    assertFilesAreIdentical(readFileName, writeFileName);
}

TEST(Image, DISABLED_ExtractImage3) {
    auto readFileName = "../../../test-files/image-3.pdf";
    pdf::Document document;
    auto result = pdf::Document::read_from_file(readFileName, document, false);
    ASSERT_FALSE(result.has_error());
    document.for_each_image([&readFileName](pdf::Image &image) {
        assertImageCanBeExtracted(image, readFileName);
        return pdf::ForEachResult::CONTINUE;
    });
}
