#pragma once

#include <cairomm/cairomm.h>

#include "document.h"
#include "operator_parser.h"

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

// TODO create matrix class or use math library for this
typedef std::array<double, 9> matrix3;

struct TextObjectState {
    // TODO initialize these matrices to the identity matrix
    matrix3 textMatrix     = {};
    matrix3 textLineMatrix = {};
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
    union {
        TrueTypeFont *trueType;
    } font;
};

struct TextState {
    double characterSpacing            = 0.0;
    double wordSpacing                 = 0.0;
    double horizontalScalingPercentage = 100;
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
    matrix3 currentTransformationMatrix = {}; // aka CTM

    double colorSpace      = {};
    Color strokingColor    = {};
    Color nonStrokingColor = {};

    TextState textState = {};

    double lineWidth  = 1.0;
    int64_t lineCap   = 0;
    int64_t lineJoin  = 0;
    double miterLimit = 10.0;
};

struct renderer {
    explicit renderer(Page *_page) : page(_page) { stateStack.emplace_back(); }

    void render(const Cairo::RefPtr<Cairo::Context> &cr);

  private:
    Page *page;
    std::vector<GraphicsState> stateStack = {};

    void render(const Cairo::RefPtr<Cairo::Context> &cr, const std::vector<Stream *> &streams);

    void setNonStrokingColor(Operator *op);
    void endText();
    void beginText();
    void pushGraphicsState();
    void popGraphicsState();
    void moveStartOfNextLine(Operator *op);
    void setTextFont(Operator *op);
    void showText(Operator *pOperator);
    void loadTrueTypeFont(TrueTypeFont *font);
};

} // namespace pdf
