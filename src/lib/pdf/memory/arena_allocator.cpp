#include "arena_allocator.h"

#include <sys/mman.h>

#include "pdf/util/debug.h"

namespace pdf {

Arena::Arena() {
    constexpr size_t GB = 1024 * 1024 * 1024;
    virtualSizeInBytes  = 64 * GB;
    bufferStart         = (uint8_t *)mmap(nullptr, virtualSizeInBytes, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    bufferPosition      = bufferStart;
    reservedSizeInBytes = 0;
}

Arena::~Arena() { munmap(bufferStart, virtualSizeInBytes); }

uint8_t *Arena::push(const size_t allocationSizeInBytes) {
    ASSERT(bufferStart != nullptr);

    if (bufferStart + reservedSizeInBytes < bufferPosition + allocationSizeInBytes) {
        const auto pageCount          = (allocationSizeInBytes / PAGE_SIZE) + 1;
        const auto allocationIncrease = PAGE_SIZE * pageCount;
        const auto err = mprotect(bufferStart + reservedSizeInBytes, allocationIncrease, PROT_READ | PROT_WRITE);
        if (err != 0) {
            // TODO handle error
            return nullptr;
        }

        reservedSizeInBytes += allocationIncrease;
    }

    const auto result = bufferPosition;
    bufferPosition += allocationSizeInBytes;
    return result;
}

void Arena::pop(const size_t allocationSizeInBytes) {
    ASSERT(bufferStart != nullptr);

    bufferPosition -= allocationSizeInBytes;
}

} // namespace pdf
