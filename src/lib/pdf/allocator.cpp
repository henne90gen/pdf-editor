#include "allocator.h"

#include <spdlog/spdlog.h>

#include "util.h"

namespace pdf {

Allocator::~Allocator() {
    while (currentAllocation != nullptr) {
        auto prev = currentAllocation->previousAllocation;
        free(currentAllocation);
        currentAllocation = prev;
    }
}

void Allocator::init(size_t sizeOfPdfFile) {
    auto sizeInBytes = sizeOfPdfFile * 2;
    auto bufferStart = (char *)malloc(sizeof(Allocation) + sizeInBytes);
    ASSERT(bufferStart != nullptr);

    currentAllocation                 = reinterpret_cast<Allocation *>(bufferStart);
    currentAllocation->sizeInBytes    = sizeInBytes;
    currentAllocation->bufferStart    = bufferStart + sizeof(Allocation);
    currentAllocation->bufferPosition = currentAllocation->bufferStart;
}

char *Allocator::allocate_chunk(size_t size) {
    ASSERT(currentAllocation->bufferStart != nullptr);
    if (currentAllocation->bufferPosition + size > currentAllocation->bufferStart + currentAllocation->sizeInBytes) {
        spdlog::warn("Failed to fit object of size {} bytes into the existing memory, allocating more", size);

        auto sizeInBytes = currentAllocation->sizeInBytes * 2;
        auto bufferStart = (char *)malloc(sizeof(Allocation) + sizeInBytes);
        ASSERT(bufferStart != nullptr);

        auto allocation                = reinterpret_cast<Allocation *>(bufferStart);
        allocation->sizeInBytes        = sizeInBytes;
        allocation->bufferStart        = bufferStart + sizeof(Allocation);
        allocation->bufferPosition     = allocation->bufferStart;
        allocation->previousAllocation = currentAllocation;
        currentAllocation              = allocation;
    }

    auto result = currentAllocation->bufferPosition;
    currentAllocation->bufferPosition += size;
    return result;
}

} // namespace pdf
