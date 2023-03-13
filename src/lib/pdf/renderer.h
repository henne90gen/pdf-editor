#pragma once

#include <cairomm/cairomm.h>

#include "pdf/operator_traverser.h"

namespace pdf {

/*
 * The Renderer expects the coordinate system to be setup like the PDF standard sets up its User Space coordinate
 * system: The positive x-axis extends horizontally to the right and the positive y-axis vertically upward. The origin
 * (0,0) of this coordinate system is located in the bottom left corner of the page.
 */
struct Renderer : public OperatorTraverser {
    const Cairo::RefPtr<Cairo::Context> &cr;
    explicit Renderer(Page &_page, const Cairo::RefPtr<Cairo::Context> &_cr) : OperatorTraverser(_page), cr(_cr) {}

    void render();

  protected:
    void on_show_text(Operator *op) override;
    void on_do(Operator *op) override;
};

} // namespace pdf
