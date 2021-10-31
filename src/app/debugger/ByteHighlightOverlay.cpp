#include "ByteHighlightOverlay.h"

#include <spdlog/spdlog.h>

#include "ContentArea.h"

void ByteHighlightOverlay::on_draw(const Cairo::RefPtr<Cairo::Context> &cr, int width, int height) const {
    spdlog::trace("ByteHighlightOverlay::on_draw(width={}, height={})", width, height);

    cr->set_source_rgb(1, 0, 0);
    int x = highlightedByte % BYTES_PER_ROW;
    int y = highlightedByte / BYTES_PER_ROW;
    cr->rectangle(x * PIXELS_PER_BYTE, y * PIXELS_PER_BYTE, PIXELS_PER_BYTE, PIXELS_PER_BYTE);
    cr->stroke();
}

void ByteHighlightOverlay::set_highlighted_byte(int b) {
    if (highlightedByte == b) {
        return;
    }

    highlightedByte = b;
    queue_draw();
}
