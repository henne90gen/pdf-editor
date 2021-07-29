#pragma once

#include "objects.h"

namespace pdf {

struct CMap {
    CMap(const std::string_view _data) : data(_data) {}
    std::string_view data;
};

struct CMapStream : public Stream {
    CMap *read_cmap();
};

} // namespace pdf
