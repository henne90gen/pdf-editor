#pragma once

#include <vector>

#include "font.h"
#include "page.h"

namespace pdf {

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

struct OperatorTraverser {
    Page &page;
    ContentStream *currentContentStream   = nullptr;
    std::vector<GraphicsState> stateStack = {};

    explicit OperatorTraverser(Page &_page) : page(_page) { stateStack.emplace_back(); }

    void traverse();

  protected:
    /// calculates the current font matrix
    [[nodiscard]] Cairo::Matrix font_matrix() const;

    GraphicsState &state() { return stateStack.back(); }
    [[nodiscard]] const GraphicsState &state() const { return stateStack.back(); }

    virtual void on_show_text(Operator *) {}
    virtual void on_do(Operator *) {}

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
};

} // namespace pdf
