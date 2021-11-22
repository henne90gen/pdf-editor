#include "page.h"

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

std::vector<TextBlock> Page::text_blocks() {
    std::vector<TextBlock> result = {};

    Font *font          = nullptr;
    auto contentStreams = content_streams();
    for (auto contentStream : contentStreams) {
        contentStream->for_each_operator(document.allocator, [this, &result, &font](Operator *op) {
            if (op->type == Operator::Type::Tf_SetTextFontAndSize) {
                auto fontOpt = get_font(op->data.Tf_SetTextFontAndSize);
                if (fontOpt.has_value()) {
                    font = fontOpt.value();
                }
            } else if (op->type == Operator::Type::Tj_ShowTextString) {
                spdlog::info("Tj_ShowTextString");
            } else if (op->type == Operator::Type::TJ_ShowOneOrMoreTextStrings) {
                spdlog::info("TJ_ShowOneOrMoreTextStrings");
                double offsetX = 0.0;
                std::string str;
                for (auto value : op->data.TJ_ShowOneOrMoreTextStrings.objects->values) {
                    if (value->is<HexadecimalString>()) {
                        spdlog::info("Found HexadecimalString");
                        auto cmapOpt = font->cmap(document);
                        if (cmapOpt.has_value()) {
                            str += cmapOpt.value()->map_char_codes(value->as<HexadecimalString>());
                        }
                    } else if (value->is<LiteralString>()) {
                        spdlog::info("Found LiteralString");
                        str += value->as<LiteralString>()->value();
                    } else if (value->is<Integer>()) {
                        spdlog::info("Found Integer");
                        offsetX -= static_cast<double>(value->as<Integer>()->value) / 1000.0;
                    }
                }

                auto extents = font->text_extents(document, str);
                result.push_back({
                      .text   = str,
                      .x      = 0.0,
                      .y      = 0.0,
                      .width  = extents.x_advance + offsetX,
                      .height = extents.y_advance,
                });
            }

            return ForEachResult::CONTINUE;
        });
    }

    return result;
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
