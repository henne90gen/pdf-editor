#include <cstdint>
#include <cstdlib>

#include <pdf/document.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    auto allocatorResult = pdf::Allocator::create();
    assert(not allocatorResult.has_error());

    auto result = pdf::Document::read_from_memory(allocatorResult.value(), data, size);
    if (result.has_error()) {
        return 0;
    }

    return 0;
}
