#pragma once

#include <unordered_map>
#include <vector>

#include "helper/allocator.h"
#include "lexer.h"
#include "objects.h"

namespace pdf {

#define ENUMERATE_OPERATION_TYPES(O)                                                                                   \
    O(UNKNOWN, UNKNOWN)                                                                                                \
    /* Text Object Operators */                                                                                        \
    O(BT, BeginText)                                                                                                   \
    O(ET, EndText)                                                                                                     \
    /* Text Positioning Operators */                                                                                   \
    O(Td, MoveStartOfNextLine)                                                                                         \
    O(TD, MoveStartOfNextLineAndSetTextLeading)                                                                        \
    O(Tm, SetTextMatrixAndTextLineMatrix)                                                                              \
    O(Tx, MoveStartOfNextLineAbsolute)                                                                                 \
    /* Text State Operators */                                                                                         \
    O(Tc, SetCharacterSpacing)                                                                                         \
    O(Tw, SetWordSpacing)                                                                                              \
    O(Tz, SetHorizontalScaling)                                                                                        \
    O(TL, SetTextLeading)                                                                                              \
    O(Tf, SetTextFontAndSize)                                                                                          \
    O(Tr, SetRenderingMode)                                                                                            \
    O(Ts, SetTextRise)                                                                                                 \
    /* Text Showing Operators */                                                                                       \
    O(Tj, ShowTextString)                                                                                              \
    O(TJ, ShowOneOrMoreTextStrings)                                                                                    \
    /*TODO O(')*/                                                                                                      \
    /*TODO O(")*/                                                                                                      \
    /* Graphics State Operators */                                                                                     \
    O(q, PushGraphicsState)                                                                                            \
    O(Q, PopGraphicsState)                                                                                             \
    O(cm, ModifyCurrentTransformationMatrix)                                                                           \
    O(w, SetLineWidth)                                                                                                 \
    O(J, SetLineCapStyle)                                                                                              \
    O(j, SetLineJoinStyle)                                                                                             \
    O(M, SetMiterLimit)                                                                                                \
    O(d, SetLineDashPattern)                                                                                           \
    O(ri, SetColorRenderingIntent)                                                                                     \
    O(i, SetFlatnessTolerance)                                                                                         \
    O(gs, SetParametersGraphicsState)                                                                                  \
    /* Path Construction Operators */                                                                                  \
    O(m, AppendNewSubpath)                                                                                             \
    O(l, AppendStraightLineSegment)                                                                                    \
    O(c, AppendCubicBezier)                                                                                            \
    O(v, AppendCubicBezier)                                                                                            \
    O(y, AppendCubicBezier)                                                                                            \
    O(h, CloseCurrentSubpathWithStraightLineSegment)                                                                   \
    O(re, AppendRectangle)                                                                                             \
    /* Path Painting Operators TODO add more descriptive names */                                                      \
    O(S, UNKNOWN)                                                                                                      \
    O(s, UNKNOWN)                                                                                                      \
    O(f, UNKNOWN)                                                                                                      \
    O(F, UNKNOWN)                                                                                                      \
    O(fx, UNKNOWN)                                                                                                     \
    O(B, UNKNOWN)                                                                                                      \
    O(Bx, UNKNOWN)                                                                                                     \
    O(b, UNKNOWN)                                                                                                      \
    O(bx, UNKNOWN)                                                                                                     \
    O(n, EndPathWithoutFillingOrStroking)                                                                              \
    /* Clipping Path Operators TODO add more descriptive names */                                                      \
    O(W, ModifyClippingPathUsingNonZeroWindingNumberRule)                                                              \
    O(Wx, ModifyClippingPathUsingEvenOddRule)                                                                          \
    /* Type 3 Font Operators TODO add more descriptive names */                                                        \
    O(d0, UNKNOWN)                                                                                                     \
    O(d1, UNKNOWN)                                                                                                     \
    /* Color Operators TODO add more descriptive names */                                                              \
    O(CS, UNKNOWN)                                                                                                     \
    O(cs, UNKNOWN)                                                                                                     \
    O(SC, UNKNOWN)                                                                                                     \
    O(SCN, UNKNOWN)                                                                                                    \
    O(sc, UNKNOWN)                                                                                                     \
    O(scn, UNKNOWN)                                                                                                    \
    O(G, SetStrokingColorGray)                                                                                         \
    O(g, SetNonStrokingColorGray)                                                                                      \
    O(RG, SetStrokingColorRGB)                                                                                         \
    O(rg, SetNonStrokingColorRGB)                                                                                      \
    O(K, UNKNOWN)                                                                                                      \
    O(k, UNKNOWN)                                                                                                      \
    /* Shading Pattern Operators TODO add more descriptive names */                                                    \
    O(sh, UNKNOWN)                                                                                                     \
    /* Inline Image Operators TODO add more descriptive names */                                                       \
    O(BI, UNKNOWN)                                                                                                     \
    O(ID, UNKNOWN)                                                                                                     \
    O(EI, UNKNOWN)                                                                                                     \
    /* XObject Operators TODO add more descriptive names */                                                            \
    O(Do, PaintXObject)                                                                                                \
    /* Marked Content Operators TODO add more descriptive names */                                                     \
    O(MP, UNKNOWN)                                                                                                     \
    O(DP, UNKNOWN)                                                                                                     \
    O(BMC, UNKNOWN)                                                                                                    \
    O(BDC, UNKNOWN)                                                                                                    \
    O(EMC, UNKNOWN)                                                                                                    \
    /* Compatibility Operators TODO add more descriptive names */                                                      \
    O(BX, UNKNOWN)                                                                                                     \
    O(EX, UNKNOWN)

struct Tf_SetTextFontSize {
    const char *fontNameData;
    size_t fontNameLength;
    double fontSize;
    [[nodiscard]] std::string_view font_name() const {
        auto fontName = std::string_view(fontNameData, fontNameLength);
        fontName      = fontName.substr(1); // remove leading "/"
        return fontName;
    }
};

struct Operator {
    enum class Type {
#define DECLARE_ENUM(Name, Description) Name##_##Description,
        ENUMERATE_OPERATION_TYPES(DECLARE_ENUM)
#undef DECLARE_ENUM
    };
    Type type;
    std::string_view content;

    union {
        struct {
            double lineWidth;
        } w_SetLineWidth;
        struct {
            std::array<double, 4> rect;
        } re_AppendRectangle;
        struct {
            double r, g, b;
        } rg_SetNonStrokingColorRGB;
        struct {
            double x, y;
        } Td_MoveStartOfNextLine;
        Tf_SetTextFontSize Tf_SetTextFontAndSize;
        struct {
            Array *objects;
        } TJ_ShowOneOrMoreTextStrings;
        struct {
            LiteralString *string;
        } Tj_ShowTextString;
        struct {
            int64_t lineCap;
        } J_LineCapStyle;
        struct {
            double matrix[6];
        } Tm_SetTextMatrixAndTextLineMatrix;
        struct {
            Name *name;
        } Do_PaintXObject;
    } data;

    explicit Operator(Type _type, std::string_view _content) : type(_type), content(_content), data() {}
};

[[maybe_unused]] Operator::Type stringToOperatorType(const std::string &t);
std::string operatorTypeToString(Operator::Type &type);
std::ostream &operator<<(std::ostream &os, Operator::Type &type);

struct OperatorParser {
    Lexer &lexer;
    Allocator &allocator;
    std::vector<Token> tokens = {};
    size_t currentTokenIdx    = 0;
    const char *lastOperatorEnd = nullptr;

    explicit OperatorParser(Lexer &_lexer, Allocator &_allocator) : lexer(_lexer), allocator(_allocator) {}

    Operator *get_operator();

  private:
    /**
     * Tries to parse the operand at the given index as the given type.
     * Indexing works backwards: operand2 operand1 operand0 OPERATOR
     */
    template <typename T> T operand(int) { ASSERT(false); }

    Operator *create_operator(Operator::Type type, std::string_view content);
    Operator *create_operator_w(Operator *result);
    Operator *create_operator_re(Operator *result);
    Operator *create_operator_rg(Operator *result);
    Operator *create_operator_Td(Operator *result);
    Operator *create_operator_Tf(Operator *result);
    Operator *create_operator_TJ(Operator *result);
    Operator *create_operator_J(Operator *result);
    Operator *create_operator_Tm(Operator *result);
    Operator *create_operator_Tj(Operator *result);
    Operator *create_operator_cm(Operator *result);
    Operator *create_operator_g(Operator *result);
    Operator *create_operator_d(Operator *result);
    Operator *create_operator_c(Operator *result);
    Operator *create_operator_s(Operator *result);
    Operator *create_operator_Tc(Operator *result);
    Operator *create_operator_CS(Operator *result);
    Operator *create_operator_S(Operator *result);
    Operator *create_operator_sc(Operator *result);
    Operator *create_operator_SC(Operator *result);
    Operator *create_operator_m(Operator *result);
    Operator *create_operator_B(Operator *result);
    Operator *create_operator_Tw(Operator *result);
    Operator *create_operator_G(Operator *result);
    Operator *create_operator_Tz(Operator *result);
    Operator *create_operator_f(Operator *result);
    Operator *create_operator_gs(Operator *result);
    Operator *create_operator_Do(Operator *result);
};

} // namespace pdf
