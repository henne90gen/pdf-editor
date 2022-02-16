#pragma once

// TODO this is a hack to get the gtkmm4 code to compile on Windows
#undef WIN32
#include <cairomm/cairomm.h>

#include "operator_traverser.h"

namespace pdf {

struct Renderer : public OperatorTraverser {
    const Cairo::RefPtr<Cairo::Context> &cr;
    explicit Renderer(Page &_page, const Cairo::RefPtr<Cairo::Context> &_cr) : OperatorTraverser(_page), cr(_cr) {}

    void render();

  protected:
    void on_show_text(Operator *op) override;
};

} // namespace pdf
