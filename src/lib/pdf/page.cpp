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

} // namespace pdf