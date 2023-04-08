#pragma once

#include <vector>

#include "pdf/font.h"

namespace pdf {

struct Page;
struct ContentStream;
struct Operator;

struct Color {
    double r = 0.0;
    double g = 0.0;
    double b = 0.0;
    double a = 0.0;

    Color() = default;

    static Color grey(double g) { return Color(g); }
    static Color rgb(double r, double g, double b) { return Color(r, g, b); }
    static Color rgba(double r, double g, double b, double a) { return Color(r, g, b, a); }

  private:
    explicit Color(double _r) : r(_r) {}
    explicit Color(double _r, double _g, double _b) : r(_r), g(_g), b(_b) {}
    explicit Color(double _r, double _g, double _b, double _a) : r(_r), g(_g), b(_b), a(_a) {}
};

struct TextObjectState {
    Cairo::Matrix textMatrix     = Cairo::identity_matrix();
    Cairo::Matrix textLineMatrix = Cairo::identity_matrix();
};

enum class TextRenderingMode {
    FILL             = 0,
    STROKE           = 1,
    FILL_STROKE      = 2,
    INVISIBLE        = 3,
    FILL_CLIP        = 4,
    STROKE_CLIP      = 5,
    FILL_STROKE_CLIP = 6,
    CLIP             = 7,
};

enum class FontType {
    TYPE0        = 0,
    TYPE1        = 1,
    MMTYPE1      = 2,
    TYPE3        = 3,
    TRUE_TYPE    = 4,
    CIDFONTTYPE0 = 5,
    CIDFONTTYPE1 = 6,
};

struct TextFont {
    FontType type;
    Font *font;
    FT_Face ftFace;
    Cairo::RefPtr<Cairo::FontFace> cairoFace;
};

struct TextState {
    double characterSpacing            = 0.0;
    double wordSpacing                 = 0.0;
    double horizontalScalingPercentage = 1.0;
    double leadingUnscaled; // TODO default unclear, not sure whether this should be a double or an int
    TextFont textFont;
    double textFontSize;
    TextRenderingMode textRenderingMode = TextRenderingMode::FILL;
    double textRiseUnscaled             = 0; // TODO not sure whether this should be a double or an int
    bool textKnockout                   = true;

    std::optional<TextObjectState> textObjectParams = {};
};

struct GraphicsState {
    // default is a matrix that converts default user coordinates to device coordinates (TODO whatever that means)
    Cairo::Matrix currentTransformationMatrix = Cairo::identity_matrix(); // aka CTM

    double colorSpace      = {};
    Color strokingColor    = {};
    Color nonStrokingColor = {};

    TextState textState = {};

    double lineWidth  = 1.0;
    int64_t lineCap   = 0;
    int64_t lineJoin  = 0;
    double miterLimit = 10.0;
};

struct TextBlock {
    Page *page = nullptr;

    std::string text;
    double x      = 0.0;
    double y      = 0.0;
    double width  = 0.0;
    double height = 0.0;

    Operator *op      = nullptr;
    ContentStream *cs = nullptr;

    /// moves the text block on the page by the specified offset
    void move(Document &document, double offsetX, double offsetY) const;
};

struct XObjectImage : public Stream {
    int64_t width() { return dictionary->must_find<Integer>("Width")->value; }
    int64_t height() { return dictionary->must_find<Integer>("Height")->value; }
    std::optional<Object *> color_space() { return dictionary->find<Object>("ColorSpace"); }
    std::optional<Integer *> bits_per_component() { return dictionary->find<Integer>("BitsPerComponent"); }
    bool image_mask() {
        const auto opt = dictionary->find<Boolean>("ImageMask");
        if (!opt.has_value()) {
            return false;
        }
        return opt.value()->value;
    }
};

struct PageImage {
    Page *page = nullptr;

    std::string name;
    double xOffset      = 0.0;
    double yOffset      = 0.0;
    XObjectImage *image = nullptr;

    Operator *op      = nullptr;
    ContentStream *cs = nullptr;

    PageImage() = default;
    PageImage(Page *_page, std::string _name, double _xOffset, double _yOffset, XObjectImage *_image, Operator *_op,
              ContentStream *_cs)
        : page(_page), name(std::move(_name)), xOffset(_xOffset), yOffset(_yOffset), image(_image), op(_op), cs(_cs) {}

    /// Moves the image on the page by the specified offset
    void move(Document &document, double offsetX, double offsetY) const;

    static ValueResult<PageImage> create(Page &page, GraphicsState &state, Operator *op, ContentStream *cs);
};

struct OperatorTraverser {
    Page &page;
    Cairo::RefPtr<Cairo::Context> cr;
    bool dirty = true;

    ContentStream *currentContentStream   = nullptr;
    std::vector<GraphicsState> stateStack = {};

    std::vector<TextBlock> textBlocks = {};
    std::vector<PageImage> images     = {};

    explicit OperatorTraverser(Page &_page, const Cairo::RefPtr<Cairo::Context> &_cr) : page(_page), cr(_cr) {
        stateStack.emplace_back();
    }

    explicit OperatorTraverser(Page &_page) : page(_page) {
        // initialize the Cairo context with a dummy surface
        auto surface = Cairo::RecordingSurface::create();
        cr           = Cairo::Context::create(surface);

        stateStack.emplace_back();
    }

    void traverse();

  protected:
    /// calculates the current font matrix
    [[nodiscard]] Cairo::Matrix font_matrix() const;

    GraphicsState &state() { return stateStack.back(); }
    [[nodiscard]] const GraphicsState &state() const { return stateStack.back(); }

  private:
    void apply_operator(Operator *op);

    void setNonStrokingColor(Operator *op);
    void endText();
    void beginText();
    void pushGraphicsState();
    void popGraphicsState();
    void moveStartOfNextLine(Operator *op);
    void setTextFontAndSize(Operator *op);
    void modifyCurrentTransformationMatrix(Operator *op);
    void endPathWithoutFillingOrStroking() const;
    void modifyClippingPathUsingEvenOddRule() const;
    void appendRectangle() const;
    void showText(Operator *);
    void onDo(Operator *);
};

} // namespace pdf
