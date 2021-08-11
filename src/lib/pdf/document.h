#pragma once

#include "cmap.h"
#include "lexer.h"
#include "objects.h"
#include "parser.h"

namespace pdf {

struct PageTreeNode;
struct Page;

struct Trailer {
    int64_t lastCrossRefStart = {};
    Dictionary *dict          = nullptr;
};

struct CrossReferenceEntry {
    int64_t byteOffset       = 0;
    int64_t generationNumber = 0;
    bool isFree              = false;
};

struct CrossReferenceTable {
    int64_t firstObjectId                    = 0;
    int64_t objectCount                      = 0;
    std::vector<CrossReferenceEntry> entries = {};
};

struct Document : public ReferenceResolver {
    char *data                              = nullptr;
    int64_t sizeInBytes                     = 0;
    Trailer trailer                         = {};
    CrossReferenceTable crossReferenceTable = {};
    std::vector<IndirectObject *> objects   = {};

    Dictionary *root();
    std::vector<Page *> pages();
    std::vector<IndirectObject *> getAllObjects();
    IndirectObject *resolve(const IndirectReference *ref) override;

    template <typename T> T *get(Object *object) {
        if (object->is<IndirectReference>()) {
            return resolve(object->as<IndirectReference>())->object->as<T>();
        } else if (object->is<IndirectObject>()) {
            return object->as<IndirectObject>()->object->as<T>();
        } else if (object->is<T>()) {
            return object->as<T>();
        }
        ASSERT(false);
        return nullptr;
    }

    template <typename T> std::optional<T *> get(std::optional<Object *> object) {
        if (object.has_value()) {
            return get<T>(object.value());
        }
        return {};
    }

    static bool load_from_file(const std::string &filePath, Document &document);

  private:
    IndirectObject *getObject(int64_t objectNumber);
    [[nodiscard]] IndirectObject *loadObject(int64_t objectNumber) const;
};

struct Rectangle : public Array {
    double getCoord(int i) {
        Object *lowerLeftX = values[i];
        if (lowerLeftX->is<Real>()) {
            return lowerLeftX->as<Real>()->value;
        } else if (lowerLeftX->is<Integer>()) {
            return static_cast<double>(lowerLeftX->as<Integer>()->value);
        } else {
            // TODO log a warning
            return 0;
        }
    }

    double lowerLeftX() { return getCoord(0); }
    double lowerLeftY() { return getCoord(1); }
    double upperRightX() { return getCoord(2); }
    double upperRightY() { return getCoord(3); }
    double width() { return upperRightX() - lowerLeftX(); }
    double height() { return upperRightY() - lowerLeftY(); }
};

struct PageTreeNode : public Dictionary {
    Name *type() { return values["Type"]->as<Name>(); }
    bool isPage() { return type()->value == "Page"; }
    PageTreeNode *parent(Document &document);
    Array *kids() { return values["Kids"]->as<Array>(); }
    Integer *count() { return values["Count"]->as<Integer>(); }

    template <typename T>
    std::optional<T *> attribute(Document &document, const std::string &attributeName, bool inheritable) {
        auto itr = values.find(attributeName);
        if (itr != values.end()) {
            return document.get<T>(itr->second);
        }

        if (!inheritable) {
            return {};
        }

        auto p = parent(document);
        if (p == nullptr) {
            return {};
        }

        const std::optional<T *> &attrib = p->attribute<T>(document, attributeName, inheritable);
        if (!attrib.has_value()) {
            return {};
        }

        return attrib.value()->template as<T>();
    }
};

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
    std::optional<Stream *> fontFile(Document &document) { return document.get<Stream>(find<Object>("FontFile")); }
    std::optional<Stream *> fontFile2(Document &document) { return document.get<Stream>(find<Object>("FontFile2")); }
    std::optional<Stream *> fontFile3(Document &document) { return document.get<Stream>(find<Object>("FontFile3")); }

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
    Array *widths(Document &document) { return document.get<Array>(values["Widths"]); }
    FontDescriptor *fontDescriptor(Document &document) {
        return document.get<FontDescriptor>(values["FontDescriptor"]);
    }
    std::optional<Object *> encoding(Document &document) { return document.get<Object>(find<Object>("Encoding")); }
    std::optional<CMapStream *> toUnicode(Document &document) {
        return document.get<CMapStream>(find<Object>("ToUnicode"));
    }
};

struct FontMap : public Dictionary {
    std::optional<Font *> get(Document &document, const std::string &fontName) {
        return document.get<Font>(find<Object>(fontName));
    }
};

struct Resources : public Dictionary {
    std::optional<FontMap *> fonts(Document &document) { return document.get<FontMap>(find<Object>("Font")); }
};

struct Page {
    explicit Page(Document &_document, PageTreeNode *_node) : document(_document), node(_node) {}

    Resources *resources() { return node->attribute<Resources>(document, "Resources", true).value(); }
    Rectangle *mediaBox() { return node->attribute<Rectangle>(document, "MediaBox", true).value(); }

    // TODO make value_or more efficient (currently mediaBox is fetched even if it is not being used)
    Rectangle *cropBox() { return node->attribute<Rectangle>(document, "CropBox", true).value_or(mediaBox()); }
    Rectangle *bleedBox() { return node->attribute<Rectangle>(document, "BleedBox", true).value_or(mediaBox()); }
    Rectangle *trimBox() { return node->attribute<Rectangle>(document, "TrimBox", true).value_or(mediaBox()); }
    Rectangle *artBox() { return node->attribute<Rectangle>(document, "ArtBox", true).value_or(mediaBox()); }
    std::optional<Dictionary *> boxColorInfo() { return node->attribute<Dictionary>(document, "BoxColorInfo", false); }
    std::optional<Object *> contents() { return node->attribute<Object>(document, "Contents", false); }
    int64_t rotate();

    Document &document;
    PageTreeNode *node;
};

} // namespace pdf
