#pragma once

#if 0
// TODO this is a hack to get the gtkmm4 code to compile on Windows
#undef WIN32
#include <cairomm/cairomm.h>
#endif

#include "operator_traverser.h"

namespace pdf {

struct Renderer : public OperatorTraverser {
#if 0
    const Cairo::RefPtr<Cairo::Context> &cr;
    explicit Renderer(Page &_page, const Cairo::RefPtr<Cairo::Context> &_cr) : OperatorTraverser(_page), cr(_cr) {}
#endif

    void render();

  protected:
#if 0
    void on_show_text(Operator *op) override;
#endif
};

} // namespace pdf
