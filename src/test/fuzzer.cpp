#include <cstdint>
#include <cstdlib>

#include <pdf/document.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    pdf::Document document;
    pdf::Document::read_from_memory(data, size, document);
    return 0;
}
