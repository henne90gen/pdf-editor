#pragma once

#include "document.h"

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

struct TextState {
    double characterSpacing            = 0.0;
    double wordSpacing                 = 0.0;
    double horizontalScalingPercentage = 100;
    double leadingUnscaled; // TODO default unclear, not sure whether this should be a double or an int
    double textFont;        // TODO
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
    explicit renderer() { stateStack.emplace_back(); }

    void render(Page *page);

  private:
    void render(const std::vector<Stream *> &streams);

    std::vector<GraphicsState> stateStack = {};
};

} // namespace pdf
