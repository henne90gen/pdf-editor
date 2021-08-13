#pragma once

#include "rectangle.h"
#include "cmap.h"

namespace pdf {

struct Document;

struct FontFlags : public Integer {
    // TODO add test for this
#define GET_NTH_BIT(n) (value & (1 << (n))) >> (n)
    bool fixedPitch() { return GET_NTH_BIT(0); }
    bool serif() { return GET_NTH_BIT(1); }
    bool symbolic() { return GET_NTH_BIT(2); }
    bool script() { return GET_NTH_BIT(3); }
    bool nonsymbolic() { return GET_NTH_BIT(5); }
    bool italic() { return GET_NTH_BIT(6); }
    bool allCap() { return GET_NTH_BIT(16); }
    bool smallCap() { return GET_NTH_BIT(17); }
    bool forceBold() { return GET_NTH_BIT(18); }
};

struct FontDescriptor : public Dictionary {
    Name *fontName() { return values["FontName"]->as<Name>(); }
    // TODO std::optional<std::string_view> fontFamily() {return find<>()}
    std::optional<Name *> fontStretch() { return find<Name>("FontStretch"); }
    std::optional<Real *> fontWeight() { return find<Real>("FontWeight"); }
    FontFlags *flags() { return values["Flags"]->as<FontFlags>(); }
    Rectangle *fontBBox() { return values["FontBBox"]->as<Rectangle>(); }
    Real *italicAngle() { return values["ItalicAngle"]->as<Real>(); }
    Real *ascent() { return values["Ascent"]->as<Real>(); }
    Real *descent() { return values["Descent"]->as<Real>(); }
    std::optional<Real *> leading() { return find<Real>("Leading"); }
    Real *capHeight() { return values["CapHeight"]->as<Real>(); }
    std::optional<Real *> xHeight() { return find<Real>("XHeight"); }
    Real *stemV() { return values["StemV"]->as<Real>(); }
    std::optional<Real *> stemH() { return find<Real>("StemH"); }
    std::optional<Real *> avgWidth() { return find<Real>("AvgWidth"); }
    std::optional<Real *> maxWidth() { return find<Real>("MaxWidth"); }
    std::optional<Real *> missingWidth() { return find<Real>("MissingWidth"); }
    std::optional<Stream *> fontFile(Document &document);
    std::optional<Stream *> fontFile2(Document &document);
    std::optional<Stream *> fontFile3(Document &document);

    /**
     * @return ASCII string or byte string
     */
    std::optional<Object *> charSet() { return find<Object>("CharSet"); }
};

struct Font : public Dictionary {
    Name *type() { return values["Subtype"]->as<Name>(); }
    bool isType0() { return type()->value == "Type0"; }
    bool isType1() { return type()->value == "Type1"; }
    bool isMMType1() { return type()->value == "MMType1"; }
    bool isType3() { return type()->value == "Type3"; }
    bool isTrueType() { return type()->value == "TrueType"; }
    bool isCIDFontType0() { return type()->value == "CIDFontType0"; }
    bool isCIDFontType2() { return type()->value == "CIDFontType2"; }
};

struct TrueTypeFont : public Font {
    std::optional<Name *> name() { return find<Name>("Name"); }
    Name *baseFont() { return values["BaseFont"]->as<Name>(); }
    Integer *firstChar() { return values["FirstChar"]->as<Integer>(); }
    Integer *lastChar() { return values["LastChar"]->as<Integer>(); }
    Array *widths(Document &document);
    FontDescriptor *fontDescriptor(Document &document);
    std::optional<Object *> encoding(Document &document);
    std::optional<CMapStream *> toUnicode(Document &document);
};

struct FontMap : public Dictionary {
    std::optional<Font *> get(Document &document, const std::string &fontName);
};

} // namespace pdf
