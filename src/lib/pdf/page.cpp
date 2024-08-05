#include "page.h"

#include <sstream>

#include "pdf/operator_parser.h"

namespace pdf {

Page::Page(Document &_document, PageTreeNode *_node) : document(_document), node(_node) {}

int64_t Page::rotate() {
    const std::optional<Integer *> &rot = node->attribute<Integer>(document, "Rotate", true);
    if (!rot.has_value()) {
        return 0;
    }
    return rot.value()->value;
}

double Page::attr_width() { return attr_crop_box()->width(); }

double Page::attr_height() { return attr_crop_box()->height(); }

std::vector<ContentStream *> Page::content_streams() {
    auto contentsOpt = attr_contents();
    if (!contentsOpt.has_value()) {
        return {};
    }

    auto content = contentsOpt.value();

    std::vector<ContentStream *> result = {};
    if (content->is<Stream>()) {
        result = {content->as<ContentStream>()};
    } else if (content->is<Array>()) {
        auto arr  = content->as<Array>();
        auto size = arr->values.size();
        result.resize(size);
        for (size_t i = 0; i < size; i++) {
            result[i] = document.get<ContentStream>(arr->values[i]);
        }
    }

    return result;
}

void ContentStream::for_each_operator(Allocator &allocator, const std::function<ForEachResult(Operator *)> &func) {
    auto decoded        = decode(allocator);
    auto textProvider   = StringTextProvider(decoded);
    auto lexer          = TextLexer(textProvider);
    auto operatorParser = OperatorParser(lexer, allocator.arena());
    Operator *op        = operatorParser.get_operator();
    while (op != nullptr) {
        ForEachResult result = func(op);
        if (result == ForEachResult::BREAK) {
            break;
        }
        op = operatorParser.get_operator();
    }
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
            const auto &str = value->as<LiteralString>()->value;
            for (size_t i = 0; i < str.size(); i++) {
                result++;
            }
        }
    }
    return result;
}

size_t count_Tj_characters(Operator *op) { return op->data.Tj_ShowTextString.string->value.size(); }

std::optional<Font *> Page::get_font(const Tf_SetTextFontSize &data) {
    auto fontMapOpt = attr_resources()->fonts(document);
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

Vector<TextBlock> Page::text_blocks() {
    traverser.traverse();
    return traverser.textBlocks;
}

Vector<PageImage> Page::images() {
    traverser.traverse();
    return traverser.images;
}

void Page::for_each_image(const std::function<ForEachResult(PageImage &)> &func) {
    auto images = this->images();
    for (auto &image : images) {
        if (func(image) == ForEachResult::BREAK) {
            break;
        }
    }
}

void Page::render(cairo_t *cr) { traverser.traverse(cr); }

} // namespace pdf
