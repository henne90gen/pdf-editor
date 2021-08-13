#pragma once

#include "objects.h"

namespace pdf {

struct Rectangle : public Array {
    double getCoord(int i) {
        Object *lowerLeftX = values[i];
        if (lowerLeftX->is<Real>()) {
            return lowerLeftX->as<Real>()->value;
        } else if (lowerLeftX->is<Integer>()) {
            return static_cast<double>(lowerLeftX->as<Integer>()->value);
        } else {
            // TODO log a warning
            return 0;
        }
    }

    double lowerLeftX() { return getCoord(0); }
    double lowerLeftY() { return getCoord(1); }
    double upperRightX() { return getCoord(2); }
    double upperRightY() { return getCoord(3); }
    double width() { return upperRightX() - lowerLeftX(); }
    double height() { return upperRightY() - lowerLeftY(); }
};

} // namespace pdf
