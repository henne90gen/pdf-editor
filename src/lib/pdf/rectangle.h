#pragma once

#include "pdf/objects.h"

namespace pdf {

struct Rectangle : public Array {
    double get_coord(int i) {
        Object *value = values[i];
        if (value->is<Real>()) {
            return value->as<Real>()->value;
        } else if (value->is<Integer>()) {
            return static_cast<double>(value->as<Integer>()->value);
        } else {
            // TODO log a warning
            return 0;
        }
    }

    double lower_left_x() { return get_coord(0); }
    double lower_left_y() { return get_coord(1); }
    double upper_right_x() { return get_coord(2); }
    double upper_right_y() { return get_coord(3); }
    double width() { return upper_right_x() - lower_left_x(); }
    double height() { return upper_right_y() - lower_left_y(); }
};

} // namespace pdf
