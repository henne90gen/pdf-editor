#include "font.h"

#include "document.h"
#include "objects.h"

namespace pdf {

std::optional<Stream *> pdf::FontDescriptor::fontFile(pdf::Document &document) {
    return document.get<Stream>(find<Object>("FontFile"));
}
std::optional<Stream *> pdf::FontDescriptor::fontFile2(pdf::Document &document) {
    return document.get<Stream>(find<Object>("FontFile2"));
}
std::optional<Stream *> pdf::FontDescriptor::fontFile3(pdf::Document &document) {
    return document.get<Stream>(find<Object>("FontFile3"));
}

std::optional<CMapStream *> pdf::TrueTypeFont::toUnicode(pdf::Document &document) {
    return document.get<CMapStream>(find<Object>("ToUnicode"));
}

pdf::FontDescriptor *pdf::TrueTypeFont::fontDescriptor(pdf::Document &document) {
    return document.get<FontDescriptor>(values["FontDescriptor"]);
}

std::optional<Object *> pdf::TrueTypeFont::encoding(pdf::Document &document) {
    return document.get<Object>(find<Object>("Encoding"));
}

pdf::Array *pdf::TrueTypeFont::widths(pdf::Document &document) { return document.get<Array>(values["Widths"]); }

std::optional<Font *> pdf::FontMap::get(pdf::Document &document, const std::string &fontName) {
    return document.get<Font>(find<Object>(fontName));
}

} // namespace pdf
