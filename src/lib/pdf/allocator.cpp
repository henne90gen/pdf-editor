#include "allocator.h"

#include <spdlog/spdlog.h>

#include "util.h"

namespace pdf {

Allocator::~Allocator() {
    size_t allocationsFreed = 0;
    size_t bytesFreed       = 0;
    auto allocation = currentAllocation;
    while (allocation != nullptr) {
        auto prev = allocation->previousAllocation;
        bytesFreed += allocation->sizeInBytes;
        free(allocation);
        allocation = prev;
        allocationsFreed++;
    }

    spdlog::trace("Freed {} allocations ({} bytes total)", allocationsFreed, bytesFreed);
}

void Allocator::init(size_t sizeOfPdfFile) {
    auto sizeInBytes = sizeof(Allocation) + sizeOfPdfFile * 2;
    auto bufferStart = (char *)malloc(sizeInBytes);
    ASSERT(bufferStart != nullptr);

    currentAllocation                     = reinterpret_cast<Allocation *>(bufferStart);
    currentAllocation->sizeInBytes        = sizeInBytes;
    currentAllocation->bufferStart        = bufferStart + sizeof(Allocation);
    currentAllocation->bufferPosition     = currentAllocation->bufferStart;
    currentAllocation->previousAllocation = nullptr;
}

void Allocator::extend(size_t size) {
    auto sizeInBytes = sizeof(Allocation) + size;
    auto bufferStart = (char *)malloc(sizeInBytes);
    ASSERT(bufferStart != nullptr);

    auto allocation                = reinterpret_cast<Allocation *>(bufferStart);
    allocation->sizeInBytes        = sizeInBytes;
    allocation->bufferStart        = bufferStart + sizeof(Allocation);
    allocation->bufferPosition     = allocation->bufferStart;
    allocation->previousAllocation = currentAllocation;
    currentAllocation              = allocation;
}

char *Allocator::allocate_chunk(size_t size) {
    ASSERT(currentAllocation->bufferStart != nullptr);
    if (currentAllocation->bufferPosition + size > currentAllocation->bufferStart + currentAllocation->sizeInBytes) {
        spdlog::trace("Failed to fit object of size {} bytes into the existing memory, allocating more", size);
        auto sizeInBytes = currentAllocation->sizeInBytes * 2;
        extend(sizeInBytes);
    }

    auto result = currentAllocation->bufferPosition;
    currentAllocation->bufferPosition += size;
    return result;
}

} // namespace pdf
