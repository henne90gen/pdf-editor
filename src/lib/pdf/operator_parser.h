#pragma once

#include <unordered_map>
#include <vector>

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
    O(Tf, SetTextFont)                                                                                                 \
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
    O(d, SetLineDashPatter)                                                                                            \
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
    O(Do, UNKNOWN)                                                                                                     \
    /* Marked Content Operators TODO add more descriptive names */                                                     \
    O(MP, UNKNOWN)                                                                                                     \
    O(DP, UNKNOWN)                                                                                                     \
    O(BMC, UNKNOWN)                                                                                                    \
    O(BDC, UNKNOWN)                                                                                                    \
    O(EMC, UNKNOWN)                                                                                                    \
    /* Compatibility Operators TODO add more descriptive names */                                                      \
    O(BX, UNKNOWN)                                                                                                     \
    O(EX, UNKNOWN)

struct Operator {
    enum class Type {
#define DECLARE_ENUM(Name, Description) Name##_##Description,
        ENUMERATE_OPERATION_TYPES(DECLARE_ENUM)
#undef DECLARE_ENUM
    };
    Type type;
    explicit Operator(Type _type) : type(_type), data() {}

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
        struct {
            const char *fontNameData;
            size_t fontNameLength;
            double fontSize;
        } Tf_SetTextFont;
        struct {
            Array *objects;
        } TJ_ShowOneOrMoreTextStrings;
    } data;
};

Operator::Type stringToOperatorType(const std::string &t);
std::ostream &operator<<(std::ostream &os, Operator::Type &type);

class OperatorParser {
  public:
    explicit OperatorParser(Lexer &_lexer) : lexer(_lexer) {}

    Operator *getOperator();

  private:
    Lexer &lexer;
    std::vector<Token> tokens = {};
    int currentTokenIdx       = 0;

    Operator *createOperator(Operator::Type type);

    Operator *createOperator_w(Operator *result);
    Operator *createOperator_re(Operator *result);
    Operator *createOperator_rg(Operator *result);
    Operator *createOperator_Td(Operator *result);
    Operator *createOperator_Tf(Operator *result);
    Operator *createOperator_TJ(Operator *result);
};

} // namespace pdf
