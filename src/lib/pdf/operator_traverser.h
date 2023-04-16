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
    // Current transformation matrix
    Cairo::Matrix ctm = Cairo::identity_matrix();

    double colorSpace      = 0.0;
    Color strokingColor    = {};
    Color nonStrokingColor = {};

    TextState textState = {};
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

    static ValueResult<PageImage> create(Page &page, const Cairo::Matrix &ctm, Operator *op, ContentStream *cs);
};

struct OperatorTraverser {
    Page &page;
    bool dirty = true;
    Cairo::RefPtr<Cairo::Surface> recordingSurface;

    ContentStream *currentContentStream   = nullptr;
    std::vector<GraphicsState> stateStack = {};

    std::vector<TextBlock> textBlocks = {};
    std::vector<PageImage> images     = {};

    explicit OperatorTraverser(Page &_page) : page(_page) { stateStack.emplace_back(); }

    void traverse() {
        // initialize the Cairo context with a dummy surface
        const auto surface = Cairo::RecordingSurface::create();
        const auto cr      = Cairo::Context::create(surface);
        traverse(cr);
    }

    void traverse(const Cairo::RefPtr<Cairo::Context> &cr);

  protected:
    GraphicsState &state() { return stateStack.back(); }
    [[nodiscard]] const GraphicsState &state() const { return stateStack.back(); }

  private:
    void apply_operator(const Cairo::RefPtr<Cairo::Context> &cr, Operator *op);

    void setNonStrokingColor(const Cairo::RefPtr<Cairo::Context> &cr, Operator *op);
    void endText();
    void beginText();
    void pushGraphicsState(const Cairo::RefPtr<Cairo::Context> &cr);
    void popGraphicsState(const Cairo::RefPtr<Cairo::Context> &cr);
    void moveStartOfNextLine(Operator *op);
    void setTextFontAndSize(const Cairo::RefPtr<Cairo::Context> &cr, Operator *op);
    void modifyCurrentTransformationMatrix(Operator *op);
    void endPathWithoutFillingOrStroking() const;
    void modifyClippingPathUsingEvenOddRule() const;
    void appendRectangle() const;
    void showText(const Cairo::RefPtr<Cairo::Context> &cr, Operator *);
    void onDo(const Cairo::RefPtr<Cairo::Context> &cr, Operator *);
};

} // namespace pdf
