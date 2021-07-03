#include "pdf_objects.h"

#include <cstring>

namespace pdf {

std::ostream &operator<<(std::ostream &os, Object::Type type) {
#define __BYTECODE_OP(op)                                                                                              \
    case Object::Type::op:                                                                                             \
        os.write(#op, strlen(#op));                                                                                    \
        break;

    switch (type) {
        ENUMERATE_OBJECT_TYPES(__BYTECODE_OP)
    default:
        ASSERT(false);
    }
#undef __BYTECODE_OP
    return os;
}

} // namespace pdf
