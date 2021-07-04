#include <gtest/gtest.h>

#include <pdf_operation_parser.h>

void assertNextOp(pdf::OperationParser &parser, pdf::Operator::Type type) {
    auto operation = parser.getOperator();
    ASSERT_NE(operation, nullptr);
    ASSERT_EQ(operation->type, type);
}

TEST(OperationParser, Simple) {
    auto textProvider = pdf::StringTextProvider("0.1 w\nq 0 0.028 611.971 791.971 re\nW* n\nQ ");
    auto lexer        = pdf::Lexer(textProvider);
    auto parser       = pdf::OperationParser(lexer);
    assertNextOp(parser, pdf::Operator::Type::w_SetLineWidth);
    assertNextOp(parser, pdf::Operator::Type::q_PushGraphicsState);
    assertNextOp(parser, pdf::Operator::Type::re_AppendRectanlge);
    assertNextOp(parser, pdf::Operator::Type::Wx_UNKNOWN);
    assertNextOp(parser, pdf::Operator::Type::n_UNKNOWN);
    assertNextOp(parser, pdf::Operator::Type::Q_PopGraphicsState);
}
