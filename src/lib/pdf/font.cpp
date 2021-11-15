#include "font.h"

#include <spdlog/spdlog.h>

#include "document.h"
#include "objects.h"

namespace pdf {

std::optional<Stream *> pdf::FontDescriptor::font_file(pdf::Document &document) {
    return document.get<Stream>(find<Object>("FontFile"));
}
std::optional<Stream *> pdf::FontDescriptor::font_file2(pdf::Document &document) {
    return document.get<Stream>(find<Object>("FontFile2"));
}
std::optional<Stream *> pdf::FontDescriptor::font_file3(pdf::Document &document) {
    return document.get<Stream>(find<Object>("FontFile3"));
}

std::optional<CMapStream *> pdf::Font::to_unicode(pdf::Document &document) {
    return document.get<CMapStream>(find<Object>("ToUnicode"));
}

pdf::FontDescriptor *pdf::Font::font_descriptor(pdf::Document &document) {
    return document.get<FontDescriptor>(values["FontDescriptor"]);
}

std::optional<Object *> pdf::Font::encoding(pdf::Document &document) {
    return document.get<Object>(find<Object>("Encoding"));
}

pdf::Array *pdf::Font::widths(pdf::Document &document) { return document.get<Array>(values["Widths"]); }

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

std::optional<Font *> pdf::FontMap::get(pdf::Document &document, const std::string &fontName) {
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

} // namespace pdf
