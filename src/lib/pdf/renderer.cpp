#include "renderer.h"

#include "operator_parser.h"
#include "util.h"

namespace pdf {

void renderer::render(const Cairo::RefPtr<Cairo::Context> &cr) {
    auto contentsOpt = page->contents();
    if (!contentsOpt.has_value()) {
        return;
    }

    // TODO set graphics state to default values
    cr->set_identity_matrix();

    auto content = contentsOpt.value();

    if (content->is<Stream>()) {
        render(cr, {content->as<Stream>()});
    } else if (content->is<Array>()) {
        auto arr     = content->as<Array>();
        auto streams = std::vector<Stream *>(arr->values.size());
        for (auto val : arr->values) {
            streams.push_back(val->as<Stream>());
        }
        render(cr, streams);
    } else {
        // NOTE this should never be reached
        ASSERT(false);
    }
}

void renderer::render(const Cairo::RefPtr<Cairo::Context> &cr, const std::vector<Stream *> &streams) {
    for (auto s : streams) {
        auto textProvider   = StringTextProvider(s->to_string());
        auto lexer          = TextLexer(textProvider);
        auto operatorParser = OperatorParser(lexer);
        Operator *op        = operatorParser.getOperator();
        while (op != nullptr) {
            if (op->type == Operator::Type::w_SetLineWidth) {
                stateStack.back().lineWidth = op->data.w_SetLineWidth.lineWidth;
            } else if (op->type == Operator::Type::q_PushGraphicsState) {
                pushGraphicsState();
            } else if (op->type == Operator::Type::Q_PopGraphicsState) {
                popGraphicsState();
            } else if (op->type == Operator::Type::re_AppendRectangle) {
                appendRectangle();
            } else if (op->type == Operator::Type::Wx_ModifyClippingPathUsingEvenOddRule) {
                modifyClippingPathUsingEvenOddRule();
            } else if (op->type == Operator::Type::n_EndPathWithoutFillingOrStroking) {
                endPathWithoutFillingOrStroking();
            } else if (op->type == Operator::Type::rg_SetNonStrokingColorRGB) {
                setNonStrokingColor(op);
            } else if (op->type == Operator::Type::BT_BeginText) {
                beginText();
            } else if (op->type == Operator::Type::ET_EndText) {
                endText();
            } else if (op->type == Operator::Type::Td_MoveStartOfNextLine) {
                moveStartOfNextLine(op);
            } else if (op->type == Operator::Type::Tf_SetTextFontAndSize) {
                setTextFontAndSize(op);
            } else if (op->type == Operator::Type::TJ_ShowOneOrMoreTextStrings) {
                showText(cr, op);
            } else {
                TODO("unknown operator");
            }
            op = operatorParser.getOperator();
        }
    }
}

void renderer::appendRectangle() const { TODO("append rectangle to the current path"); }

void renderer::modifyClippingPathUsingEvenOddRule() const { TODO("clipping path modification"); }

void renderer::endPathWithoutFillingOrStroking() const {
    // TODO this is a path painting no-op
    //  it does however set the clipping path, if a clipping path operator was used before it
    TODO("path painting no-op");
}

void renderer::setNonStrokingColor(Operator *op) {
    stateStack.back().nonStrokingColor = Color::rgb( //
          op->data.rg_SetNonStrokingColorRGB.r,      //
          op->data.rg_SetNonStrokingColorRGB.g,      //
          op->data.rg_SetNonStrokingColorRGB.b       //
    );
}

void renderer::endText() {
    // make sure text object parameters are set before unsetting them
    ASSERT(stateStack.back().textState.textObjectParams.has_value());
    stateStack.back().textState.textObjectParams = {};
}

void renderer::beginText() {
    // only one BT ... ET can be open at a time
    ASSERT(!stateStack.back().textState.textObjectParams.has_value());
    stateStack.back().textState.textObjectParams = std::optional(TextObjectState());
}

void renderer::pushGraphicsState() { stateStack.emplace_back(); }

void renderer::popGraphicsState() { stateStack.pop_back(); }

void renderer::moveStartOfNextLine(Operator *op) { TODO("implement move start of next line"); }

void renderer::setTextFontAndSize(Operator *op) {
    stateStack.back().textState.textFontSize = op->data.Tf_SetTextFontAndSize.fontSize;

    auto fontName =
          std::string_view(op->data.Tf_SetTextFontAndSize.fontNameData, op->data.Tf_SetTextFontAndSize.fontNameLength);
    fontName        = fontName.substr(1); // remove leading "/"
    auto fontMapOpt = page->resources()->fonts(page->document);
    if (!fontMapOpt.has_value()) {
        TODO("logging");
        return;
    }

    auto fontOpt = fontMapOpt.value()->get(page->document, std::string(fontName));
    if (!fontOpt.has_value()) {
        TODO("logging");
        return;
    }

    auto font = fontOpt.value();

    if (font->isTrueType()) {
        loadTrueTypeFont(font->as<TrueTypeFont>());
    } else {
        TODO("implement loading of other fonts");
        ASSERT(false);
    }
}

void renderer::showText(const Cairo::RefPtr<Cairo::Context> &cr, Operator *op) {
    auto &textState = stateStack.back().textState;

    auto values                      = op->data.TJ_ShowOneOrMoreTextStrings.objects->values;
    std::vector<Cairo::Glyph> glyphs = {};
    double xOffset                   = 0;
    for (auto value : values) {
        if (value->is<Integer>()) {
            auto i = value->as<Integer>();
            std::cout << "Integer " << i->value << std::endl;
            //            xOffset += static_cast<double>(i->value) / 1000.0;
        } else if (value->is<HexadecimalString>()) {
            std::cout << "HexadecimalString: ";
            auto str = value->as<HexadecimalString>()->to_string();
            std::cout << str.length() << " | ";
            for (char c : str) {
                auto i = static_cast<unsigned long>(c);
                std::cout << i << " ";
                // TODO apply T_rise as y offset
                glyphs.push_back({.index = i, .x = xOffset, .y = 0.0});
                xOffset += 10;

                // TODO get text extents and adjust xOffset accordingly
                //                textState.textFont.ftFace->charmap;
                //                Cairo::TextExtents extents;
                //                cr->get_text_extents("H", extents);
                //                xOffset += extents.x_advance;
            }
            std::cout << std::endl;
        }
    }

    auto matrix = Cairo::identity_matrix();
    matrix.translate(100, 100);
    matrix.scale(textState.textFontSize, textState.textFontSize);

    cr->set_source_rgb(0.0, 0.0, 0.0);
    cr->set_font_matrix(matrix);
    cr->set_font_face(textState.textFont.cairoFace);
    cr->show_glyphs(glyphs);
}

void renderer::loadTrueTypeFont(TrueTypeFont *font) {
    stateStack.back().textState.textFont.type     = FontType::TRUE_TYPE;
    stateStack.back().textState.textFont.trueType = font;

    std::cout << "Base Font: " << font->baseFont()->value << std::endl;
    std::cout << "First Char: " << font->firstChar()->value << std::endl;
    std::cout << "Last Char: " << font->lastChar()->value << std::endl;

    std::cout << "Widths: ";
    for (auto &value : font->widths(page->document)->values) {
        std::cout << value->as<Integer>()->value << ", ";
    }
    std::cout << std::endl;

    std::cout << "Font Descriptor:" << std::endl;
    for (auto &entry : font->fontDescriptor(page->document)->values) {
        std::cout << "  " << entry.first << ": " << entry.second << std::endl;
    }

    std::cout << "Has Name: " << font->name().has_value() << std::endl;
    std::cout << "Has Encoding: " << font->encoding(page->document).has_value() << std::endl;
    auto toUnicodeOpt = font->toUnicode(page->document);
    std::cout << "Has ToUnicode: " << toUnicodeOpt.has_value() << std::endl;
    // TODO getting the ToUnicode table crashes the following object load operation
    if (toUnicodeOpt.has_value()) {
        auto toUnicode          = toUnicodeOpt.value();
        const char *cmapFilePtr = toUnicode->to_string().data();
        // TODO read in cmap file
    }

    std::optional<Stream *> fontFileOpt = font->fontDescriptor(page->document)->fontFile2(page->document);
    if (!fontFileOpt.has_value()) {
        std::cerr << "Could not find embedded font file" << std::endl;
        return;
    }

    auto fontFile = fontFileOpt.value();

    int64_t faceIndex = 0;
    FT_Library library;
    auto error = FT_Init_FreeType(&library);
    if (error != FT_Err_Ok) {
        std::cerr << "Failed to initialize freetype!" << std::endl;
        return;
    }

    FT_Face face;
    auto view    = fontFile->to_string();
    auto basePtr = view.data();
    auto size    = (int64_t)view.length();
    error        = FT_New_Memory_Face(library, reinterpret_cast<const FT_Byte *>(basePtr), size, faceIndex, &face);
    if (error != FT_Err_Ok) {
        std::cerr << "Failed to load font" << std::endl;
        return;
    }
    std::cout << "Num Glyphs: " << face->num_glyphs << std::endl;

    stateStack.back().textState.textFont.ftFace = face;
    stateStack.back().textState.textFont.cairoFace =
          Cairo::RefPtr(new Cairo::FontFace(cairo_ft_font_face_create_for_ft_face(face, 0)));

    for (int i = 0; i < face->num_charmaps; i++) {
        FT_Set_Charmap(face, face->charmaps[i]);
        for (unsigned long code = 1; code <= 0xFFFF; code++) {
            unsigned long glyph_index = FT_Get_Char_Index(face, code);
            /* 0 = .notdef */
            if (glyph_index != 0) {
                std::cout << glyph_index << " - " << code << std::endl;
            }
        }
    }
}

} // namespace pdf
