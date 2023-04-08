#include "page.h"

#include <sstream>

#include "pdf/operator_parser.h"
#include "pdf/operator_traverser.h"

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

std::vector<TextBlock> Page::text_blocks() {
    // TODO cache the results of this
    auto traverser = OperatorTraverser(*this);
    traverser.traverse();
    return traverser.textBlocks;
}

std::vector<PageImage> Page::images() {
    // TODO cache the results of this
    auto traverser = OperatorTraverser(*this);
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

void PageImage::move(Document &document, double offsetX, double offsetY) const {
    std::stringstream ss;

    auto decoded            = cs->decode(document.allocator);
    auto bytesUntilOperator = op->content.data() - decoded.data();

    // write everything up to the operator we want to wrap
    ss << decoded.substr(0, bytesUntilOperator);

    if (op->content[0] != ' ') {
        ss << " ";
    }

    // TODO operator stream, if we applied such a move operation previously and just edit that one instead

    // save state
    ss << "q ";

    // apply offset
    ss << "1 0 0 1 ";
    ss << offsetX;
    ss << " ";
    ss << offsetY;
    ss << " cm ";

    // write operator
    ss << op->content;

    // restore state
    ss << " Q";

    ss << decoded.substr(bytesUntilOperator + op->content.size());

    cs->encode(document.allocator, ss.str());

    spdlog::info("Moved image '{}' by x={} and y={}", name, offsetX, offsetY);
}

void TextBlock::move(Document &document, double offsetX, double offsetY) const {
    // BT 56.8 724.1 Td /F1 12 Tf            [<01>-2<02>1<03>2<03>2<0405>17<06>76<040708>]TJ              ET Q Q
    // BT 56.8 724.1 Td /F1 12 Tf _x_ _y_ Td [<01>-2<02>1<03>2<03>2<0405>17<06>76<040708>]TJ -_x_ -_y_ Td ET Q Q
    std::stringstream ss;

    // write everything up to the operator we want to wrap
    auto decoded = cs->decode(document.allocator);
    ss << decoded.substr(0, op->content.data() - decoded.data());

    // wrap operator with offset
    if (op->content[0] != ' ') {
        ss << " ";
    }
    ss << offsetX;
    ss << " ";
    ss << -offsetY;
    ss << " Td ";

    // write operator
    ss << op->content;

    // wrap operator with negative offset
    ss << " ";
    ss << -offsetX;
    ss << " ";
    ss << offsetY;
    ss << " Td ";

    ss << decoded.substr(op->content.data() - decoded.data() + op->content.size());

    cs->encode(document.allocator, ss.str());

    spdlog::info("Moved text block '{}' by x={} and y={}", text, offsetX, offsetY);
}

ValueResult<PageImage> PageImage::create(Page &page, GraphicsState &state, Operator *op, ContentStream *cs) {
    const auto &xObjectName = op->data.Do_PaintXObject.name->value;

    const auto xObjectMapOpt = page.resources()->x_objects(page.document);
    if (!xObjectMapOpt.has_value()) {
        return ValueResult<PageImage>::error("asda");
    }

    const auto &xObjectMap   = xObjectMapOpt.value();
    const auto &xObjectKey   = xObjectName.substr(1, xObjectName.size() - 1);
    const auto xObjectRefOpt = xObjectMap->find<IndirectReference>(xObjectKey);
    const auto xObjectOpt    = page.document.get<Stream>(xObjectRefOpt);
    if (!xObjectOpt.has_value()) {
        return ValueResult<PageImage>::error("Failed to find XObject with XObjectKey={}", xObjectKey);
    }

    const auto xObject = xObjectOpt.value();
    const auto subtype = xObject->dictionary->must_find<Name>("Subtype");
    if (subtype->value != "Image") {
        return ValueResult<PageImage>::error("XObject is not an image");
    }

    double xOffset = 0.0;
    double yOffset = 0.0;
    state.currentTransformationMatrix.transform_point(xOffset, yOffset);
    auto pageImage = PageImage(&page, xObjectKey, xOffset, yOffset, xObject->as<XObjectImage>(), op, cs);
    return ValueResult<PageImage>::ok(pageImage);
}

} // namespace pdf
