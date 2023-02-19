#include "font.h"

#include <spdlog/spdlog.h>

#include "pdf/document.h"
#include "pdf/objects.h"

namespace pdf {

std::optional<Stream *> FontDescriptor::font_file(Document &document) {
    return document.get<Stream>(find<Object>("FontFile"));
}
std::optional<Stream *> FontDescriptor::font_file2(Document &document) {
    return document.get<Stream>(find<Object>("FontFile2"));
}
std::optional<Stream *> FontDescriptor::font_file3(Document &document) {
    return document.get<Stream>(find<Object>("FontFile3"));
}

std::optional<CMapStream *> Font::to_unicode(Document &document) {
    return document.get<CMapStream>(find<Object>("ToUnicode"));
}

std::optional<FontDescriptor *> Font::font_descriptor(Document &document) {
    auto objectOpt = find<Object>("FontDescriptor");
    if (!objectOpt.has_value()) {
        spdlog::warn("Failed to find FontDescriptor in dictionary:");
        for (auto &itr : values) {
            spdlog::warn("    key={}, value={}", itr.first, (int)itr.second->type);
        }
        return nullptr;
    }

    return document.get<FontDescriptor>(objectOpt.value());
}

std::optional<Object *> Font::encoding(Document &document) { return document.get<Object>(find<Object>("Encoding")); }

Array *Font::widths(Document &document) { return document.get<Array>(must_find<Object>("Widths")); }

std::optional<Stream *> Font::font_program(Document &document) {
    const std::optional<FontDescriptor *> &fontDescriptor = font_descriptor(document);
    if (is_true_type()) {
        if (!fontDescriptor.has_value()) {
            return {};
        }
        return fontDescriptor.value()->font_file2(document);
    }

    if (is_type1()) {
        if (!fontDescriptor.has_value()) {
            return {};
        }
        return fontDescriptor.value()->font_file(document);
    }

    // TODO Get font program for this type of font
    return {};
}

std::optional<Font *> FontMap::get(Document &document, const std::string &fontName) {
    return document.get<Font>(find<Object>(fontName));
}

std::optional<CMap *> Font::cmap(Document &document) {
    // FIXME cache cmap object

    auto cmapStreamOpt = to_unicode(document);
    if (!cmapStreamOpt.has_value()) {
        return {};
    }

    return cmapStreamOpt.value()->read_cmap(document.arena);
}

FT_Face Font::load_font_face(Document &document) {
    auto fontFileOpt = font_program(document);
    if (!fontFileOpt.has_value()) {
        spdlog::error("Failed to find embedded font program!");
        return nullptr;
    }

    auto fontFile = fontFileOpt.value();

    int64_t faceIndex = 0;
    FT_Library library;
    auto error = FT_Init_FreeType(&library);
    if (error != FT_Err_Ok) {
        spdlog::error("Failed to initialize freetype!");
        return nullptr;
    }

    FT_Face face;
    auto view    = fontFile->decode(document.arena);
    auto basePtr = view.data();
    auto size    = (int64_t)view.length();
    error        = FT_New_Memory_Face(library, reinterpret_cast<const FT_Byte *>(basePtr), size, faceIndex, &face);
    if (error != FT_Err_Ok) {
        spdlog::error("Failed to load embedded font program!");
        return nullptr;
    }

    return face;
}

} // namespace pdf
