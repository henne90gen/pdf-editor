#include "page.h"

namespace pdf {

int64_t Page::rotate() {
    const std::optional<Integer *> &rot = node->attribute<Integer>(document, "Rotate", true);
    if (!rot.has_value()) {
        return 0;
    }
    return rot.value()->value;
}

double Page::width() { return cropBox()->width(); }

double Page::height() { return cropBox()->height(); }

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
        for (int i = 0; i < arr->values.size(); i++) {
            result[i] = document.get<ContentStream>(arr->values[i]);
        }
    }

    return result;
}

void ContentStream::for_each_operator(const std::function<bool(Operator*)> &func) {
    auto textProvider   = StringTextProvider(to_string());
    auto lexer          = TextLexer(textProvider);
    auto operatorParser = OperatorParser(lexer);
    Operator *op        = operatorParser.getOperator();
    while (op != nullptr) {
        if (!func(op)){
            break;
        }
        op = operatorParser.getOperator();
    }
}

} // namespace pdf
