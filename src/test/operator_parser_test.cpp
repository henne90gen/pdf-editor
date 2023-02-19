#include <functional>
#include <gtest/gtest.h>

#include <pdf/operator_parser.h>

void assertNextOp(pdf::OperatorParser &parser, pdf::Operator::Type type,
                  const std::function<void(pdf::Operator *)> &func) {
    auto operation = parser.get_operator();
    ASSERT_NE(operation, nullptr);
    ASSERT_EQ(operation->type, type);
    func(operation);
}

void assertNextOp(pdf::OperatorParser &parser, pdf::Operator::Type type) {
    assertNextOp(parser, type, [](auto /*op*/) {});
}

TEST(OperationParser, Simple) {
    auto textProvider = pdf::StringTextProvider("0.1 w\nq 0 0.028 611.971 791.971 re\nW* n\nQ ");
    auto lexer        = pdf::TextLexer(textProvider);
    auto arena        = pdf::Arena();
    auto parser       = pdf::OperatorParser(lexer, arena);
    assertNextOp(parser, pdf::Operator::Type::w_SetLineWidth,
                 [](auto op) { ASSERT_EQ(op->data.w_SetLineWidth.lineWidth, 0.1); });
    assertNextOp(parser, pdf::Operator::Type::q_PushGraphicsState);
    assertNextOp(parser, pdf::Operator::Type::re_AppendRectangle, [](auto op) {
        ASSERT_FLOAT_EQ(op->data.re_AppendRectangle.rect[0], 0);
        ASSERT_FLOAT_EQ(op->data.re_AppendRectangle.rect[1], 0.028);
        ASSERT_FLOAT_EQ(op->data.re_AppendRectangle.rect[2], 611.971);
        ASSERT_FLOAT_EQ(op->data.re_AppendRectangle.rect[3], 791.971);
    });
    assertNextOp(parser, pdf::Operator::Type::Wx_ModifyClippingPathUsingEvenOddRule);
    assertNextOp(parser, pdf::Operator::Type::n_EndPathWithoutFillingOrStroking);
    assertNextOp(parser, pdf::Operator::Type::Q_PopGraphicsState);
}

TEST(OperationParser, HelloWorld) {
    auto textProvider =
          pdf::StringTextProvider("0.1 w\nq 0 0.028 611.971 791.971 re\nW* n\nq 0 0 0 rg\nBT\n56.8 724.1 Td /F1 12 "
                                  "Tf[<01>-2<02>1<03>2<03>2<0405>17<06>76<040708>]TJ\nET\nQ\nQ ");
    auto lexer  = pdf::TextLexer(textProvider);
    auto arena  = pdf::Arena();
    auto parser = pdf::OperatorParser(lexer, arena);
    assertNextOp(parser, pdf::Operator::Type::w_SetLineWidth,
                 [](auto op) { ASSERT_EQ(op->data.w_SetLineWidth.lineWidth, 0.1); });
    assertNextOp(parser, pdf::Operator::Type::q_PushGraphicsState);
    assertNextOp(parser, pdf::Operator::Type::re_AppendRectangle, [](auto op) {
        ASSERT_FLOAT_EQ(op->data.re_AppendRectangle.rect[0], 0);
        ASSERT_FLOAT_EQ(op->data.re_AppendRectangle.rect[1], 0.028);
        ASSERT_FLOAT_EQ(op->data.re_AppendRectangle.rect[2], 611.971);
        ASSERT_FLOAT_EQ(op->data.re_AppendRectangle.rect[3], 791.971);
    });
    assertNextOp(parser, pdf::Operator::Type::Wx_ModifyClippingPathUsingEvenOddRule);
    assertNextOp(parser, pdf::Operator::Type::n_EndPathWithoutFillingOrStroking);
    assertNextOp(parser, pdf::Operator::Type::q_PushGraphicsState);
    assertNextOp(parser, pdf::Operator::Type::rg_SetNonStrokingColorRGB, [](auto op) {
        ASSERT_FLOAT_EQ(op->data.rg_SetNonStrokingColorRGB.r, 0.0);
        ASSERT_FLOAT_EQ(op->data.rg_SetNonStrokingColorRGB.r, 0.0);
        ASSERT_FLOAT_EQ(op->data.rg_SetNonStrokingColorRGB.r, 0.0);
    });
    assertNextOp(parser, pdf::Operator::Type::BT_BeginText);
    assertNextOp(parser, pdf::Operator::Type::Td_MoveStartOfNextLine, [](auto op) {
        ASSERT_FLOAT_EQ(op->data.Td_MoveStartOfNextLine.x, 56.8);
        ASSERT_FLOAT_EQ(op->data.Td_MoveStartOfNextLine.y, 724.1);
    });
    assertNextOp(parser, pdf::Operator::Type::Tf_SetTextFontAndSize, [](auto op) {
        auto fontName = std::string_view(op->data.Tf_SetTextFontAndSize.fontNameData,
                                         op->data.Tf_SetTextFontAndSize.fontNameLength);
        ASSERT_EQ(fontName, "/F1");
        ASSERT_EQ(op->data.Tf_SetTextFontAndSize.fontSize, 12);
    });
    assertNextOp(parser, pdf::Operator::Type::TJ_ShowOneOrMoreTextStrings, [](auto op) {
        pdf::Array *objects = op->data.TJ_ShowOneOrMoreTextStrings.objects;
        ASSERT_NE(objects, nullptr);
        ASSERT_EQ(objects->values.size(), 13);
        // [<01>-2<02>1<03>2<03>2<0405>17<06>76<040708>]
        ASSERT_EQ(objects->values[0]->as<pdf::HexadecimalString>()->value, "01");
        ASSERT_EQ(objects->values[1]->as<pdf::Integer>()->value, -2);
        ASSERT_EQ(objects->values[2]->as<pdf::HexadecimalString>()->value, "02");
        ASSERT_EQ(objects->values[3]->as<pdf::Integer>()->value, 1);
        ASSERT_EQ(objects->values[4]->as<pdf::HexadecimalString>()->value, "03");
        ASSERT_EQ(objects->values[5]->as<pdf::Integer>()->value, 2);
        ASSERT_EQ(objects->values[6]->as<pdf::HexadecimalString>()->value, "03");
        ASSERT_EQ(objects->values[7]->as<pdf::Integer>()->value, 2);
        ASSERT_EQ(objects->values[8]->as<pdf::HexadecimalString>()->value, "0405");
        ASSERT_EQ(objects->values[9]->as<pdf::Integer>()->value, 17);
        ASSERT_EQ(objects->values[10]->as<pdf::HexadecimalString>()->value, "06");
        ASSERT_EQ(objects->values[11]->as<pdf::Integer>()->value, 76);
        ASSERT_EQ(objects->values[12]->as<pdf::HexadecimalString>()->value, "040708");
    });
    assertNextOp(parser, pdf::Operator::Type::ET_EndText);
    assertNextOp(parser, pdf::Operator::Type::Q_PopGraphicsState);
    assertNextOp(parser, pdf::Operator::Type::Q_PopGraphicsState);
}

TEST(OperationParser, Content) {
    auto textProvider = pdf::StringTextProvider("0.1 w\nq 0 0.028 611.971 791.971 re\nW* n\nQ ");
    auto lexer        = pdf::TextLexer(textProvider);
    auto arena        = pdf::Arena();
    auto parser       = pdf::OperatorParser(lexer, arena);
    assertNextOp(parser, pdf::Operator::Type::w_SetLineWidth, [](auto op) { ASSERT_EQ(op->content, "0.1 w"); });
    assertNextOp(parser, pdf::Operator::Type::q_PushGraphicsState, [](auto op) { ASSERT_EQ(op->content, "q"); });
    assertNextOp(parser, pdf::Operator::Type::re_AppendRectangle,
                 [](auto op) { ASSERT_EQ(op->content, "0 0.028 611.971 791.971 re"); });
    assertNextOp(parser, pdf::Operator::Type::Wx_ModifyClippingPathUsingEvenOddRule,
                 [](auto op) { ASSERT_EQ(op->content, "W*"); });
    assertNextOp(parser, pdf::Operator::Type::n_EndPathWithoutFillingOrStroking,
                 [](auto op) { ASSERT_EQ(op->content, "n"); });
    assertNextOp(parser, pdf::Operator::Type::Q_PopGraphicsState, [](auto op) { ASSERT_EQ(op->content, "Q"); });
}
