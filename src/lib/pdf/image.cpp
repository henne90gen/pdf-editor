#include "image.h"

#include <cmath>
#include <cstring>
#include <fstream>

namespace pdf {

#pragma pack(push, 1)
struct BmpFileHeader {
    uint16_t identifier = 0x4D42;
    uint32_t fileSizeInBytes;
    uint16_t reserved0   = 0;
    uint16_t reserved1   = 0;
    uint32_t pixelOffset = 54;
};

struct BmpInfoHeader {
    uint32_t size = 40;
    int32_t width;
    int32_t height;
    uint16_t numPlanes    = 1;
    uint16_t bitsPerPixel = 24;
    uint32_t compression  = 0;
    uint32_t imageSize    = 0;
    uint32_t xPixelsPerM;
    uint32_t yPixelsPerM;
    uint32_t colorsUsed;
    uint32_t importantColors = 0;
};

struct BmpFile {
    BmpFileHeader fileHeader;
    BmpInfoHeader infoHeader;
    uint8_t *pixels = nullptr;
};
#pragma pack(pop)

bool Image::write_bmp(const std::string &fileName) const {
    BmpFileHeader fileHeader   = {};
    fileHeader.fileSizeInBytes = fileHeader.pixelOffset + width * height * 3;

    BmpInfoHeader infoHeader = {};
    infoHeader.width         = static_cast<int32_t>(width);
    infoHeader.height        = static_cast<int32_t>(height);
    infoHeader.bitsPerPixel  = 3 * stream->dictionary->values["BitsPerComponent"]->as<Integer>()->value;
    infoHeader.xPixelsPerM   = 0;
    infoHeader.yPixelsPerM   = 0;
    infoHeader.colorsUsed    = 0;

    auto pixels = stream->decode(allocator);

    auto rowSize = static_cast<int32_t>(std::ceil((infoHeader.bitsPerPixel * width) / 32)) * 4;
    auto pBuf    = (uint8_t *)pixels.data();
    if (rowSize != width * 3) {
        // we have to create a copy of the pixels to account for additional padding
        // that is necessary at the end of each pixel row

        auto pixelsSize = static_cast<int32_t>(rowSize * height);
        pBuf            = reinterpret_cast<uint8_t *>(std::malloc(pixelsSize));
        std::memset(pBuf, 0, pixelsSize);

        // TODO this could be optimized with omp and memcpy (copying each row
        //  instead of each pixel)
        for (int row = 0; row < height; row++) {
            for (int col = 0; col < width; col++) {
                auto pBufIndex      = row * rowSize + col * 3;
                auto pixelsIndex    = static_cast<int32_t>(row * width + col) * 3;
                pBuf[pBufIndex + 0] = pixels[pixelsIndex + 0];
                pBuf[pBufIndex + 1] = pixels[pixelsIndex + 1];
                pBuf[pBufIndex + 2] = pixels[pixelsIndex + 2];
            }
        }
    }

    BmpFile bmpFile = {
          fileHeader,
          infoHeader,
          pBuf,
    };

    std::ofstream file(fileName, std::ios::binary);

    size_t headerSize = sizeof(bmpFile.fileHeader) + sizeof(bmpFile.infoHeader);
    file.write(reinterpret_cast<const char *>(&bmpFile), static_cast<std::streamsize>(headerSize));

    uint32_t pixelSize = bmpFile.fileHeader.fileSizeInBytes - bmpFile.fileHeader.pixelOffset;
    file.write(reinterpret_cast<const char *>(bmpFile.pixels), pixelSize);

    file.flush();
    file.close();

    return true;
}

} // namespace pdf
