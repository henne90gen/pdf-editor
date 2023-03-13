#include <cstdint>
#include <cstdlib>

#include <pdf/document.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    auto result = pdf::Document::read_from_memory(data, size);
    if (result.has_error()) {
        return 0;
    }

    result.value().destroy();
    return 0;
}
