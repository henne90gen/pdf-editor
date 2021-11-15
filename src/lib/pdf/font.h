#pragma once

#include "cmap.h"
#include "rectangle.h"

namespace pdf {

struct Document;

struct FontFlags : public Integer {
#define GET_NTH_BIT(n) (value & (1 << (n))) >> (n)
    bool fixed_pitch() { return GET_NTH_BIT(0); }
    bool serif() { return GET_NTH_BIT(1); }
    bool symbolic() { return GET_NTH_BIT(2); }
    bool script() { return GET_NTH_BIT(3); }
    bool non_symbolic() { return GET_NTH_BIT(5); }
    bool italic() { return GET_NTH_BIT(6); }
    bool all_cap() { return GET_NTH_BIT(16); }
    bool small_cap() { return GET_NTH_BIT(17); }
    bool force_bold() { return GET_NTH_BIT(18); }
};

struct FontDescriptor : public Dictionary {
    Name *font_name() { return values["FontName"]->as<Name>(); }
    // TODO std::optional<std::string_view> fontFamily() {return find<>()}
    std::optional<Name *> font_stretch() { return find<Name>("FontStretch"); }
    std::optional<Real *> font_weight() { return find<Real>("FontWeight"); }
    FontFlags *flags() { return values["Flags"]->as<FontFlags>(); }
    Rectangle *font_bbox() { return values["FontBBox"]->as<Rectangle>(); }
    Real *italic_angle() { return values["ItalicAngle"]->as<Real>(); }
    Real *ascent() { return values["Ascent"]->as<Real>(); }
    Real *descent() { return values["Descent"]->as<Real>(); }
    std::optional<Real *> leading() { return find<Real>("Leading"); }
    Real *cap_height() { return values["CapHeight"]->as<Real>(); }
    std::optional<Real *> x_height() { return find<Real>("XHeight"); }
    Real *stem_v() { return values["StemV"]->as<Real>(); }
    std::optional<Real *> stem_h() { return find<Real>("StemH"); }
    std::optional<Real *> avg_width() { return find<Real>("AvgWidth"); }
    std::optional<Real *> max_width() { return find<Real>("MaxWidth"); }
    std::optional<Real *> missing_width() { return find<Real>("MissingWidth"); }
    std::optional<Stream *> font_file(Document &document);
    std::optional<Stream *> font_file2(Document &document);
    std::optional<Stream *> font_file3(Document &document);

    /**
     * @return ASCII string or byte string
     */
    std::optional<Object *> char_set() { return find<Object>("CharSet"); }
};

struct Font : public Dictionary {
    std::string_view type() { return values["Subtype"]->as<Name>()->value(); }
    bool is_type0() { return type() == "Type0"; }
    bool is_type1() { return type() == "Type1"; }
    bool is_MM_type1() { return type() == "MMType1"; }
    bool is_type3() { return type() == "Type3"; }
    bool is_true_type() { return type() == "TrueType"; }
    bool is_CID_font_type0() { return type() == "CIDFontType0"; }
    bool is_CID_font_type2() { return type() == "CIDFontType2"; }
    std::optional<Name *> name() { return find<Name>("Name"); }
    Name *base_font() { return values["BaseFont"]->as<Name>(); }
    Integer *first_char() { return values["FirstChar"]->as<Integer>(); }
    Integer *last_char() { return values["LastChar"]->as<Integer>(); }
    Array *widths(Document &document);
    FontDescriptor *font_descriptor(Document &document);
    std::optional<Object *> encoding(Document &document);
    std::optional<CMapStream *> to_unicode(Document &document);
    std::optional<Stream *> font_program(Document &document);

    /// Character mapping
    std::optional<CMap *> cmap(Document &document);
};

struct FontMap : public Dictionary {
    std::optional<Font *> get(Document &document, const std::string &fontName);
};

} // namespace pdf
