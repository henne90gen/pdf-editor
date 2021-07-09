#pragma once

#include <unordered_map>
#include <vector>

#include "pdf_lexer.h"

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
    O(n, UNKNOWN)                                                                                                      \
    /* Clipping Path Operators TODO add more descriptive names */                                                      \
    O(W, UNKNOWN)                                                                                                      \
    O(Wx, UNKNOWN)                                                                                                     \
    /* Unsorted Operators TODO add more descriptive names and find headlines */                                        \
    O(Tj, ShowTextString)                                                                                              \
    O(TJ, ShowOneOrMoreTextStrings)                                                                                    \
    /*TODO O(')*/                                                                                                      \
    /*TODO O(")*/                                                                                                      \
    O(d0, UNKNOWN)                                                                                                     \
    O(d1, UNKNOWN)                                                                                                     \
    O(CS, UNKNOWN)                                                                                                     \
    O(SC, UNKNOWN)                                                                                                     \
    O(SCN, UNKNOWN)                                                                                                    \
    O(sc, UNKNOWN)                                                                                                     \
    O(scn, UNKNOWN)                                                                                                    \
    O(G, SetStrokingColorGray)                                                                                         \
    O(g, SetNonStrokingColorGray)                                                                                      \
    O(RG, SetStrokingColorRGB)                                                                                         \
    O(rg, SetNonStrokingColorRGB)                                                                                      \
    O(K, UNKNOWN)                                                                                                      \
    O(k, UNKNOWN)

// sh
// BI, ID, EI
// Do
// MP, DP, BMC, BDC, EMC
// BX, EX

struct Operator {
    enum class Type {
#define DECLARE_ENUM(Name, Description) Name##_##Description,
        ENUMERATE_OPERATION_TYPES(DECLARE_ENUM)
#undef DECLARE_ENUM
    };
    Type type;
    explicit Operator(Type _type) : type(_type) {}

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
            char *fontName;
            size_t fontName_length;
            double fontSize; // TODO is font size a "real" or an "integer"
        } Tf_SetTextFont;
    } data;
};

Operator::Type stringToOperatorType(const std::string &t);

class OperationParser {
  public:
    explicit OperationParser(Lexer &_lexer) : lexer(_lexer) {}

    Operator *getOperator();

  private:
    Lexer &lexer;
    std::vector<Token> tokens = {};
    int currentTokenIdx       = 0;

    Operator *createOperator(Operator::Type type);
};

} // namespace pdf
