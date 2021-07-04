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
    O(re, AppendRectanlge)                                                                                             \
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
    O(Wx, UNKNOWN)

// Tj, TJ, ', "
// d0, d1
// CS, cs, SC, SCN, sc, scn, G, g, RG, rg, K, k
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
};

Operator::Type stringToOperatorType(const std::string &t);

// NOTE: Use this code to generate the struct definitions for all operators
//#define STRUCT(Name, Description) \
//    struct Name##_##Description : public Operator {};
// ENUMERATE_OPERATION_TYPES(STRUCT)
//#undef STRUCT

/* Text Object Operators */
struct BT_BeginText : public Operator {};
struct ET_EndText : public Operator {};
/* Text Positioning Operators */
struct Td_MoveStartOfNextLine : public Operator {};
struct TD_MoveStartOfNextLineAndSetTextLeading : public Operator {};
struct Tm_SetTextMatrixAndTextLineMatrix : public Operator {};
struct Tx_MoveStartOfNextLineAbsolute : public Operator {};
/* Text State Operators */
struct Tc_SetCharacterSpacing : public Operator {};
struct Tw_SetWordSpacing : public Operator {};
struct Tz_SetHorizontalScaling : public Operator {};
struct TL_SetTextLeading : public Operator {};
struct Tf_SetTextFont : public Operator {};
struct Tr_SetRenderingMode : public Operator {};
struct Ts_SetTextRise : public Operator {};
/* Graphics State Operators */
struct q_PushGraphicsState : public Operator {};
struct Q_PopGraphicsState : public Operator {};
struct cm_ModifyCurrentTransformationMatrix : public Operator {};
struct w_SetLineWidth : public Operator {
    explicit w_SetLineWidth(double _lineWidth) : Operator(Operator::Type::w_SetLineWidth), lineWidth(_lineWidth) {}
    double lineWidth = 0.0;
};
struct J_SetLineCapStyle : public Operator {};
struct j_SetLineJoinStyle : public Operator {};
struct M_SetMiterLimit : public Operator {};
struct d_SetLineDashPatter : public Operator {};
struct ri_SetColorRenderingIntent : public Operator {};
struct i_SetFlatnessTolerance : public Operator {};
struct gs_SetParametersGraphicsState : public Operator {};
/* Path Construction Operators */
struct m_AppendNewSubpath : public Operator {};
struct l_AppendStraightLineSegment : public Operator {};
struct c_AppendCubicBezier : public Operator {};
struct v_AppendCubicBezier : public Operator {};
struct y_AppendCubicBezier : public Operator {};
struct h_CloseCurrentSubpathWithStraightLineSegment : public Operator {};
struct re_AppendRectanlge : public Operator {};
/* Path Painting Operators */
struct S_UNKNOWN : public Operator {};
struct s_UNKNOWN : public Operator {};
struct f_UNKNOWN : public Operator {};
struct F_UNKNOWN : public Operator {};
struct fx_UNKNOWN : public Operator {};
struct B_UNKNOWN : public Operator {};
struct Bx_UNKNOWN : public Operator {};
struct b_UNKNOWN : public Operator {};
struct bx_UNKNOWN : public Operator {};
struct n_UNKNOWN : public Operator {};
/* Clipping Path Operators */
struct W_UNKNOWN : public Operator {};
struct Wx_UNKNOWN : public Operator {};

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
