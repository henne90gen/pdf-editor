#pragma once

#include <cairomm/cairomm.h>

#include "pdf/operator_traverser.h"

namespace pdf {

struct Renderer : public OperatorTraverser {
    const Cairo::RefPtr<Cairo::Context> &cr;
    explicit Renderer(Page &_page, const Cairo::RefPtr<Cairo::Context> &_cr) : OperatorTraverser(_page), cr(_cr) {}

    void render();

  protected:
    void on_show_text(Operator *op) override;
};

} // namespace pdf
