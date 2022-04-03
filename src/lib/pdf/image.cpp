#include "image.h"

#include <cmath>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>

namespace pdf {

#pragma pack(push, 1)
struct BmpFileHeader {
    /// 00  0 2 - The header field used to identify the BMP and DIB file is 0x42 0x4D in hexadecimal, same as 'BM' in
    /// ASCII.
    uint16_t identifier = 0x4D42;
    /// 02  2 4 - The size of the BMP file in bytes
    uint32_t fileSizeInBytes;
    /// 06  6 2 - Reserved; actual value depends on the application that creates the image, if created manually can be 0
    uint16_t reserved0 = 0;
    /// 08  8 2 - Reserved; actual value depends on the application that creates the image, if created manually can be 0
    uint16_t reserved1 = 0;
    /// 0A 10 4 - The offset, i.e. starting address, of the byte where the bitmap image data (pixel array) can be found.
    uint32_t pixelOffset = 54;
};

struct BmpInfoHeader {
    /// 0E 14 4 - the size of this header, in bytes (40)
    uint32_t size = 40;
    /// 12 18 4 - the bitmap width in pixels (signed integer)
    int32_t width;
    /// 16 22 4 - the bitmap height in pixels (signed integer)
    int32_t height;
    /// 1A 26 2 - the number of color planes (must be 1)
    uint16_t numPlanes = 1;
    /// 1C 28 2 - the number of bits per pixel, which is the color depth of the image. Typical values are 1, 4, 8, 16,
    /// 24 and 32.
    uint16_t bitsPerPixel = 24;
    /// 1E 30 4 - the compression method being used. See the next table for a list of possible values
    uint32_t compression = 0;
    /// 22 34 4 - the image size. This is the size of the raw bitmap data; a dummy 0 can be given for BI_RGB bitmaps.
    uint32_t imageSize = 0;
    /// 26 38 4 - the horizontal resolution of the image. (pixel per metre, signed integer)
    int32_t xPixelsPerM;
    /// 2A 42 4 - the vertical resolution of the image. (pixel per metre, signed integer)
    int32_t yPixelsPerM;
    /// 2E 46 4 - the number of colors in the color palette, or 0 to default to 2n
    uint32_t colorsUsed;
    /// 32 50 4 - the number of important colors used, or 0 when every color is important; generally ignored
    uint32_t importantColors = 0;
};
#pragma pack(pop)

Result write_bmp(const std::string &fileName, int32_t width, int32_t height, uint32_t bitsPerComponent,
                 std::string_view pixels) {

    BmpInfoHeader infoHeader = {};
    infoHeader.width         = width;
    infoHeader.height        = height;
    infoHeader.bitsPerPixel  = 3 * bitsPerComponent;
    infoHeader.xPixelsPerM   = 0;
    infoHeader.yPixelsPerM   = 0;
    infoHeader.colorsUsed    = 0;

    switch (infoHeader.bitsPerPixel) {
    case 1:
    case 4:
    case 8:
    case 16:
    case 24:
    case 32:
        break;
    default:
        return Result::error("Invalid value for bits per pixel (allowed values: 1, 4, 8, 16, 24, 32): {}",
                             infoHeader.bitsPerPixel);
    }

    auto currentRowSize = static_cast<int32_t>((infoHeader.bitsPerPixel * width) / 32.0 * 4.0);
    auto paddedRowSize  = static_cast<int32_t>(std::ceil((infoHeader.bitsPerPixel * width) / 32.0)) * 4;
    auto pixelsSize     = static_cast<size_t>(paddedRowSize * height);
    ASSERT(pixels.size() == static_cast<size_t>(currentRowSize * height));

    auto pBuf = reinterpret_cast<uint8_t *>(std::malloc(pixelsSize));
    std::memset(pBuf, 0, pixelsSize);

    auto padding = paddedRowSize - currentRowSize;

    spdlog::trace("Adding {} bytes of padding to the end of each pixel row", padding);

    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            auto pBufIndex      = (height - row - 1) * paddedRowSize + col * 3;
            auto pixelsIndex    = static_cast<int32_t>(row * currentRowSize + col * 3);
            pBuf[pBufIndex + 0] = pixels[pixelsIndex + 2];
            pBuf[pBufIndex + 1] = pixels[pixelsIndex + 1];
            pBuf[pBufIndex + 2] = pixels[pixelsIndex + 0];
        }
    }

    std::ofstream file(fileName, std::ios::out | std::ios::binary);
    if (!file) {
        return Result::error("Failed to open file '{}' for writing", fileName);
    }

    BmpFileHeader fileHeader   = {};
    fileHeader.fileSizeInBytes = fileHeader.pixelOffset + (paddedRowSize * height);
    file.write(reinterpret_cast<const char *>(&fileHeader), sizeof(fileHeader));
    if (file.bad()) {
        return Result::error("Failed to write to file '{}'", fileName);
    }
    file.write(reinterpret_cast<const char *>(&infoHeader), sizeof(infoHeader));
    if (file.bad()) {
        return Result::error("Failed to write to file '{}'", fileName);
    }

    file.write(reinterpret_cast<const char *>(pBuf), static_cast<int64_t>(pixelsSize));
    if (file.bad()) {
        return Result::error("Failed to write to file '{}'", fileName);
    }

    std::free(pBuf);

    return Result::ok();
}

Result Image::write_bmp(const std::string &fileName) const {
    auto pixels = stream->decode(allocator);
    return ::pdf::write_bmp(fileName, static_cast<int32_t>(width), static_cast<int32_t>(height), bitsPerComponent,
                            pixels);
}

ValueResult<Image *> Image::read_bmp(Allocator &allocator, const std::string &fileName) {
    std::ifstream file(fileName, std::ios::in | std::ios::binary);
    if (file.fail() || !file.is_open()) {
        return ValueResult<Image *>::error("Failed to open image file: {}", fileName);
    }

    BmpFileHeader fileHeader;
    size_t fileHeaderSize = sizeof(fileHeader);
    file.read(reinterpret_cast<char *>(&fileHeader), fileHeaderSize);

    BmpInfoHeader infoHeader;
    size_t infoHeaderSize = sizeof(infoHeader);
    file.read(reinterpret_cast<char *>(&infoHeader), infoHeaderSize);

    uint32_t pixelSize = fileHeader.fileSizeInBytes - fileHeader.pixelOffset;
    auto pixels        = reinterpret_cast<uint8_t *>(std::malloc(pixelSize));
    file.read(reinterpret_cast<char *>(pixels), pixelSize);

    // TODO flip BGR to RGB and remove padding

    auto image              = allocator.allocate<Image>(allocator);
    image->width            = infoHeader.width;
    image->height           = infoHeader.height;
    image->bitsPerComponent = infoHeader.bitsPerPixel / 3;

    auto imageDict = std::unordered_map<std::string, Object *>();
    image->stream  = Stream::create_from_unencoded_data(allocator, imageDict,
                                                        std::string_view(reinterpret_cast<char *>(pixels), pixelSize));

    return ValueResult<Image *>::ok(image);
}

} // namespace pdf
