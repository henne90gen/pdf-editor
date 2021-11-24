#include "page.h"

#include "operator_traverser.h"

namespace pdf {

int64_t Page::rotate() {
    const std::optional<Integer *> &rot = node->attribute<Integer>(document, "Rotate", true);
    if (!rot.has_value()) {
        return 0;
    }
    return rot.value()->value;
}

double Page::width() { return crop_box()->width(); }

double Page::height() { return crop_box()->height(); }

std::vector<ContentStream *> Page::content_streams() {
    auto contentsOpt = contents();
    if (!contentsOpt.has_value()) {
        return {};
    }

    auto content = contentsOpt.value();

    std::vector<ContentStream *> result = {};
    if (content->is<Stream>()) {
        result = {content->as<ContentStream>()};
    } else if (content->is<Array>()) {
        auto arr = content->as<Array>();
        result.resize(arr->values.size());
        for (size_t i = 0; i < arr->values.size(); i++) {
            result[i] = document.get<ContentStream>(arr->values[i]);
        }
    }

    return result;
}

void ContentStream::for_each_operator(Allocator &allocator, const std::function<ForEachResult(Operator *)> &func) {
    auto textProvider   = StringTextProvider(decode(allocator));
    auto lexer          = TextLexer(textProvider);
    auto operatorParser = OperatorParser(lexer, allocator);
    Operator *op        = operatorParser.get_operator();
    while (op != nullptr) {
        ForEachResult result = func(op);
        if (result == ForEachResult::BREAK) {
            break;
        }
        op = operatorParser.get_operator();
    }
}

struct TextBlockFinder : public OperatorTraverser {
    std::vector<TextBlock> result = {};

    explicit TextBlockFinder(Page &page) : OperatorTraverser(page) {}

    std::vector<TextBlock> find() {
        traverse();
        return result;
    }

    void on_show_text(Operator *op) override {
        spdlog::info("TJ_ShowOneOrMoreTextStrings");

        TextFont &textFont = state().textState.textFont;
        auto cmapOpt       = textFont.font->cmap(page.document);

        auto face       = textFont.cairoFace;
        auto fontMatrix = font_matrix();
        auto ctm        = stateStack.back().currentTransformationMatrix;
        auto scaledFont = Cairo::ScaledFont::create(face, fontMatrix, ctm);

        auto glyphs     = std::vector<Cairo::Glyph>();
        double offsetX = 0.0;
        std::string text;
        for (auto value : op->data.TJ_ShowOneOrMoreTextStrings.objects->values) {
            if (value->is<HexadecimalString>()) {
                spdlog::info("Found HexadecimalString");
                if (cmapOpt.has_value()) {
                    text += cmapOpt.value()->map_char_codes(value->as<HexadecimalString>());
                }
                auto str = value->as<HexadecimalString>()->to_string();
                for (char c : str) {
                    auto i             = static_cast<uint8_t>(c);
                    Cairo::Glyph glyph = {.index = i, .x = offsetX, .y = 0.0};
                    glyphs.push_back(glyph);

                    Cairo::TextExtents extents;
                    cairo_scaled_font_glyph_extents(scaledFont->cobj(), &glyph, 1, &extents);
                    offsetX += static_cast<double>(extents.x_advance);
                }
            } else if (value->is<LiteralString>()) {
                spdlog::info("Found LiteralString");
                text += value->as<LiteralString>()->value();
            } else if (value->is<Integer>()) {
                spdlog::info("Found Integer");
                offsetX -= static_cast<double>(value->as<Integer>()->value) / 1000.0;
            }
        }

        double x = 0.0;
        double y = 0.0;
        fontMatrix.transform_point(x, y);

        // FIXME width and height calculation are not correct (only x_advance is filled out in extents struct)
        Cairo::TextExtents extents = {};
        cairo_scaled_font_glyph_extents(scaledFont->cobj(), glyphs.data(), glyphs.size(), &extents);
        result.push_back({
              .text   = text,
              .x      = x,
              .y      = y,
              .width  = extents.width,
              .height = extents.height,
        });
    }
};

std::vector<TextBlock> Page::text_blocks() {
    auto finder = TextBlockFinder(*this);
    return finder.find();
}

size_t count_TJ_characters(CMap *cmap, Operator *op) {
    // TODO skip whitespace characters
    size_t result = 0;
    for (auto value : op->data.TJ_ShowOneOrMoreTextStrings.objects->values) {
        if (value->is<Integer>()) {
            // do nothing
        } else if (value->is<HexadecimalString>()) {
            result += cmap->map_char_codes(value->as<HexadecimalString>()).size();
        } else if (value->is<LiteralString>()) {
            auto str = std::string(value->as<LiteralString>()->value());
            for (size_t i = 0; i < str.size(); i++) {
                result++;
            }
        }
    }
    return result;
}

size_t count_Tj_characters(Operator *op) { return op->data.Tj_ShowTextString.string->value().size(); }

std::optional<Font *> Page::get_font(const Tf_SetTextFontSize &data) {
    auto fontMapOpt = resources()->fonts(document);
    if (!fontMapOpt.has_value()) {
        // TODO add logging
        return {};
    }

    auto fontName = std::string(data.font_name());
    auto fontOpt  = fontMapOpt.value()->get(document, fontName);
    if (!fontOpt.has_value()) {
        // TODO add logging
        return {};
    }

    return fontOpt.value();
}

size_t Page::character_count() {
    size_t result       = 0;
    auto contentStreams = content_streams();
    CMap *cmap          = nullptr;
    for (auto contentStream : contentStreams) {
        contentStream->for_each_operator(document.allocator, [&result, this, &cmap](Operator *op) {
            if (op->type == Operator::Type::TJ_ShowOneOrMoreTextStrings) {
                result += count_TJ_characters(cmap, op);
            } else if (op->type == Operator::Type::Tj_ShowTextString) {
                result += count_Tj_characters(op);
            } else if (op->type == Operator::Type::Tf_SetTextFontAndSize) {
                auto fontOpt = get_font(op->data.Tf_SetTextFontAndSize);
                if (fontOpt.has_value()) {
                    auto font    = fontOpt.value();
                    auto cmapOpt = font->cmap(document);
                    if (cmapOpt.has_value()) {
                        cmap = cmapOpt.value();
                    }
                }
            }

            // TODO also count other text operators

            return ForEachResult::CONTINUE;
        });
    }
    return result;
}

} // namespace pdf
