#include "arena_allocator.h"

#if WIN32
#define ReserveAddressRange(sizeInBytes) 0;
#define ReleaseAddressRange(buffer, sizeInBytes) void()
#else
#include <sys/mman.h>
#define ReserveAddressRange(sizeInBytes)                                                                               \
    (uint8_t *)mmap(nullptr, sizeInBytes, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)
#define ReleaseAddressRange(buffer, sizeInBytes) munmap(buffer, sizeInBytes)
#endif

namespace pdf {

Arena::Arena() {
    constexpr size_t GB = 1024 * 1024 * 1024;
    init(64 * GB);
}

Arena::Arena(const size_t maximumSizeInBytes, const size_t pageSize) {
    this->pageSize = pageSize;
    init(maximumSizeInBytes);
}

Arena::~Arena() { ReleaseAddressRange(bufferStart, virtualSizeInBytes); }

void Arena::init(size_t maximumSizeInBytes) {
    virtualSizeInBytes  = maximumSizeInBytes;
    bufferStart         = ReserveAddressRange(virtualSizeInBytes);
    bufferPosition      = bufferStart;
    reservedSizeInBytes = 0;
}

uint8_t *Arena::push(size_t allocationSizeInBytes) {
    ASSERT(bufferStart != nullptr);

    if (bufferStart + reservedSizeInBytes < bufferPosition + allocationSizeInBytes) {
        const auto pageCount          = (allocationSizeInBytes / pageSize) + 1;
        const auto allocationIncrease = pageSize * pageCount;
        ASSERT(allocationIncrease + reservedSizeInBytes <= virtualSizeInBytes);
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

void Arena::pop(size_t allocationSizeInBytes) {
    ASSERT(bufferStart != nullptr);
    bufferPosition -= allocationSizeInBytes;
}

void Arena::pop_all() { bufferPosition = bufferStart; }

void TemporaryArena::start_using() {
    ASSERT(!isInUse);
    isInUse = true;
}

void TemporaryArena::stop_using() {
    isInUse = false;
    arena.pop_all();
}

} // namespace pdf
