#include <functional>
#include <gtest/gtest.h>

#include <pdf_operation_parser.h>

void assertNextOp(pdf::OperationParser &parser, pdf::Operator::Type type, std::function<void(pdf::Operator *)> func) {
    auto operation = parser.getOperator();
    ASSERT_NE(operation, nullptr);
    ASSERT_EQ(operation->type, type);
    func(operation);
}

TEST(OperationParser, Simple) {
    auto textProvider = pdf::StringTextProvider("0.1 w\nq 0 0.028 611.971 791.971 re\nW* n\nQ ");
    auto lexer        = pdf::Lexer(textProvider);
    auto parser       = pdf::OperationParser(lexer);
    assertNextOp(parser, pdf::Operator::Type::w_SetLineWidth,
                 [](auto op) { ASSERT_EQ(op->data.w_SetLineWidth.lineWidth, 0.1); });
    assertNextOp(parser, pdf::Operator::Type::q_PushGraphicsState, [](auto op) {});
    assertNextOp(parser, pdf::Operator::Type::re_AppendRectangle, [](auto op) {
        ASSERT_FLOAT_EQ(op->data.re_AppendRectangle.rect[0], 0);
        ASSERT_FLOAT_EQ(op->data.re_AppendRectangle.rect[1], 0.028);
        ASSERT_FLOAT_EQ(op->data.re_AppendRectangle.rect[2], 611.971);
        ASSERT_FLOAT_EQ(op->data.re_AppendRectangle.rect[3], 791.971);
    });
    assertNextOp(parser, pdf::Operator::Type::Wx_UNKNOWN, [](auto op) {});
    assertNextOp(parser, pdf::Operator::Type::n_UNKNOWN, [](auto op) {});
    assertNextOp(parser, pdf::Operator::Type::Q_PopGraphicsState, [](auto op) {});
}

TEST(OperationParser, HelloWorld) {
    auto textProvider =
          pdf::StringTextProvider("0.1 w\nq 0 0.028 611.971 791.971 re\nW* n\nq 0 0 0 rg\nBT\n56.8 724.1 Td /F1 12 "
                                  "Tf[<01>-2<02>1<03>2<03>2<0405>17<06>76<040708>]TJ\nET\nQ\nQ ");
    auto lexer  = pdf::Lexer(textProvider);
    auto parser = pdf::OperationParser(lexer);
    assertNextOp(parser, pdf::Operator::Type::w_SetLineWidth,
                 [](auto op) { ASSERT_EQ(op->data.w_SetLineWidth.lineWidth, 0.1); });
    assertNextOp(parser, pdf::Operator::Type::q_PushGraphicsState, [](auto op) {});
    assertNextOp(parser, pdf::Operator::Type::re_AppendRectangle, [](auto op) {
        ASSERT_FLOAT_EQ(op->data.re_AppendRectangle.rect[0], 0);
        ASSERT_FLOAT_EQ(op->data.re_AppendRectangle.rect[1], 0.028);
        ASSERT_FLOAT_EQ(op->data.re_AppendRectangle.rect[2], 611.971);
        ASSERT_FLOAT_EQ(op->data.re_AppendRectangle.rect[3], 791.971);
    });
    assertNextOp(parser, pdf::Operator::Type::Wx_UNKNOWN, [](auto op) {});
    assertNextOp(parser, pdf::Operator::Type::n_UNKNOWN, [](auto op) {});
    assertNextOp(parser, pdf::Operator::Type::q_PushGraphicsState, [](auto op) {});
    assertNextOp(parser, pdf::Operator::Type::rg_SetNonStrokingColorRGB, [](auto op) {
        ASSERT_FLOAT_EQ(op->data.rg_SetNonStrokingColorRGB.r, 0.0);
        ASSERT_FLOAT_EQ(op->data.rg_SetNonStrokingColorRGB.r, 0.0);
        ASSERT_FLOAT_EQ(op->data.rg_SetNonStrokingColorRGB.r, 0.0);
    });
    assertNextOp(parser, pdf::Operator::Type::BT_BeginText, [](auto op) {});
    assertNextOp(parser, pdf::Operator::Type::Td_MoveStartOfNextLine, [](auto op) {
        ASSERT_FLOAT_EQ(op->data.Td_MoveStartOfNextLine.x, 56.8);
        ASSERT_FLOAT_EQ(op->data.Td_MoveStartOfNextLine.y, 724.1);
    });
    assertNextOp(parser, pdf::Operator::Type::Tf_SetTextFont,
                 [](auto op) { ASSERT_EQ(op->data.Tf_SetTextFont.fontSize, 12); });
    assertNextOp(parser, pdf::Operator::Type::TJ_ShowOneOrMoreTextStrings, [](auto op) {
        // TODO extend this test
    });
    assertNextOp(parser, pdf::Operator::Type::ET_EndText, [](auto op) {});
    assertNextOp(parser, pdf::Operator::Type::Q_PopGraphicsState, [](auto op) {});
    assertNextOp(parser, pdf::Operator::Type::Q_PopGraphicsState, [](auto op) {});
}
