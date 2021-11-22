#include "font.h"

#include <spdlog/spdlog.h>

#include "document.h"
#include "objects.h"

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

FontDescriptor *Font::font_descriptor(Document &document) {
    return document.get<FontDescriptor>(must_find<Object>("FontDescriptor"));
}

std::optional<Object *> Font::encoding(Document &document) { return document.get<Object>(find<Object>("Encoding")); }

Array *Font::widths(Document &document) { return document.get<Array>(must_find<Object>("Widths")); }

std::optional<Stream *> Font::font_program(Document &document) {
    if (is_true_type()) {
        return font_descriptor(document)->font_file2(document);
    }
    if (is_type1()) {
        return font_descriptor(document)->font_file(document);
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

    return cmapStreamOpt.value()->read_cmap(document.allocator);
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
    auto view    = fontFile->decode(document.allocator);
    auto basePtr = view.data();
    auto size    = (int64_t)view.length();
    error        = FT_New_Memory_Face(library, reinterpret_cast<const FT_Byte *>(basePtr), size, faceIndex, &face);
    if (error != FT_Err_Ok) {
        spdlog::error("Failed to load embedded font program!");
        return nullptr;
    }

    return face;
}

Cairo::TextExtents Font::text_extents(Document &document, const std::string &text) {
    auto face       = load_font_face(document);
    auto ftFace     = Cairo::FtFontFace::create(face, 0);
    auto fontMatrix = Cairo::identity_matrix();
    auto ctm        = Cairo::identity_matrix();
    auto scaledFont = Cairo::ScaledFont::create(ftFace, fontMatrix, ctm);
    // TODO apply correct font matrix
    Cairo::TextExtents extents;
    cairo_scaled_font_text_extents(scaledFont->cobj(), text.c_str(), &extents);
    return extents;
}

} // namespace pdf
