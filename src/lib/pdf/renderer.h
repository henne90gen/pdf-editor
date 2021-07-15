#pragma once

#include "document.h"

namespace pdf {

struct GraphicsState {
    std::array<double, 9> currentTransformationMatrix; // aka CTM
    double lineWidth;
};

struct renderer {
    explicit renderer() { stateStack.emplace_back(); }

    void render(Page *page);

  private:
    void render(const std::vector<Stream *> &streams);

    std::vector<GraphicsState> stateStack = {};
};

} // namespace pdf
