#include "arena_allocator.h"

namespace pdf {

#if WIN32
#include <memoryapi.h>
PtrResult ReserveAddressRange(size_t sizeInBytes) {
    const auto ptr = (uint8_t *)VirtualAlloc(nullptr, sizeInBytes, MEM_RESERVE, PAGE_READWRITE);
    if (ptr == nullptr) {
        return PtrResult::error("failed to reserve address range of size {}", sizeInBytes);
    }
    return PtrResult::ok(ptr);
};
Result ReleaseAddressRange(uint8_t *buffer, size_t sizeInBytes) {
    (void)buffer;
    (void)sizeInBytes;
    return Result::ok();
}
Result ReserveMemory(uint8_t *buffer, size_t sizeInBytes) {
    // TODO check for errors
    VirtualAlloc(buffer, sizeInBytes, MEM_COMMIT, PAGE_READWRITE);
    return Result::ok();
}
#else
#include <sys/mman.h>
PtrResult ReserveAddressRange(size_t sizeInBytes) {
    // TODO check for error on mmap
    const auto ptr = (uint8_t *)mmap(nullptr, sizeInBytes, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    return PtrResult::ok(ptr);
};
Result ReleaseAddressRange(uint8_t *buffer, size_t sizeInBytes) {
    // TODO check for error on munmap
    munmap(buffer, sizeInBytes);
    return Result::ok();
}
Result ReserveMemory(uint8_t *buffer, size_t sizeInBytes) {
    const auto err = mprotect(buffer, sizeInBytes, PROT_READ | PROT_WRITE);
    if (err != 0) {
        // TODO create more concrete error message
        return Result::error("failed to reserve memory of size {}", sizeInBytes);
    }
    return Result::ok();
}
#endif

ValueResult<Arena> Arena::create() {
    constexpr size_t GB = 1024 * 1024 * 1024;
    return create(128 * GB);
}

ValueResult<Arena> Arena::create(size_t maximumSizeInBytes, size_t pageSize) {
    auto result = ReserveAddressRange(maximumSizeInBytes);
    if (result.has_error()) {
        return ValueResult<Arena>::of(result.drop_value());
    }
    return ValueResult<Arena>::ok(Arena(result.value(), maximumSizeInBytes, pageSize));
}

Arena::Arena(uint8_t *_buffer, const size_t _maximumSizeInBytes, const size_t _pageSize) {
    pageSize            = _pageSize;
    virtualSizeInBytes  = _maximumSizeInBytes;
    bufferStart         = _buffer;
    bufferPosition      = bufferStart;
    reservedSizeInBytes = 0;
}

Arena::~Arena() { ReleaseAddressRange(bufferStart, virtualSizeInBytes); }

uint8_t *Arena::push(size_t allocationSizeInBytes) {
    ASSERT(bufferStart != nullptr);

    if (bufferStart + reservedSizeInBytes < bufferPosition + allocationSizeInBytes) {
        const auto pageCount          = (allocationSizeInBytes / pageSize) + 1;
        const auto allocationIncrease = pageSize * pageCount;
        ASSERT(allocationIncrease + reservedSizeInBytes <= virtualSizeInBytes);

        const auto result = ReserveMemory(bufferStart + reservedSizeInBytes, allocationIncrease);
        if (result.has_error()) {
            spdlog::error(result.message());
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

} // namespace pdf
