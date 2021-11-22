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

void ContentStream::for_each_operator(Allocator &allocator, const std::function<bool(Operator *)> &func) {
    auto textProvider   = StringTextProvider(decode(allocator));
    auto lexer          = TextLexer(textProvider);
    auto operatorParser = OperatorParser(lexer, allocator);
    Operator *op        = operatorParser.get_operator();
    while (op != nullptr) {
        if (!func(op)) {
            break;
        }
        op = operatorParser.get_operator();
    }
}

std::vector<TextBlock> Page::text_blocks() {
    std::vector<TextBlock> result = {};

    auto contentStreams = content_streams();
    for (auto contentStream : contentStreams) {
        contentStream->for_each_operator(document.allocator, [&result](Operator *op) {
            if (op->type == Operator::Type::Tj_ShowTextString) {
                spdlog::info("Tj_ShowTextString");
            } else if (op->type == Operator::Type::TJ_ShowOneOrMoreTextStrings) {
                spdlog::info("TJ_ShowOneOrMoreTextStrings");
                std::string str;
                for (auto elem : op->data.TJ_ShowOneOrMoreTextStrings.objects->values) {
                    if (elem->is<HexadecimalString>()) {
                        spdlog::info("Found HexadecimalString");
                    } else if (elem->is<LiteralString>()) {
                        spdlog::info("Found LiteralString");
                        str += elem->as<LiteralString>()->value();
                    } else if (elem->is<Integer>()) {
                        spdlog::info("Found Integer");
                    }
                }
                result.push_back({
                      .text   = str,
                      .x      = 0.0,
                      .y      = 0.0,
                      .width  = 0.0,
                      .height = 0.0,
                });
            }
            return true;
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
            auto codes = value->as<HexadecimalString>()->to_string();
            for (char code : codes) {
                auto strOpt = cmap->map_char_code(code);
                if (strOpt.has_value()) {
                    result += strOpt.value().size();
                }
            }
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
                auto fontMapOpt = resources()->fonts(document);
                if (!fontMapOpt.has_value()) {
                    // TODO add logging
                    return true;
                }

                auto fontName = std::string(op->data.Tf_SetTextFontAndSize.font_name());
                auto fontOpt  = fontMapOpt.value()->get(document, fontName);
                if (!fontOpt.has_value()) {
                    // TODO add logging
                    return true;
                }

                auto font    = fontOpt.value();
                auto cmapOpt = font->cmap(document);
                if (!cmapOpt.has_value()) {
                    return true;
                }
                cmap = cmapOpt.value();
            }

            // TODO also count other text operators

            return true;
        });
    }
    return result;
}

} // namespace pdf
