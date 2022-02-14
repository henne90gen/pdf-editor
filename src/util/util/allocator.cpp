#include "allocator.h"

#include <spdlog/spdlog.h>

#include "util.h"

namespace util {

Allocator::~Allocator() {
    size_t allocationsFreed = 0;
    size_t bytesFreed       = 0;

    for_each_allocation([&allocationsFreed, &bytesFreed](Allocation &allocation) {
        bytesFreed += allocation.sizeInBytes;
        free(&allocation);
        allocationsFreed++;
        return util::ForEachResult::CONTINUE;
    });

    spdlog::trace("Freed {} allocations ({} bytes total)", allocationsFreed, bytesFreed);
}

void Allocator::init(size_t sizeOfPdfFile) {
    if (sizeOfPdfFile == 0) {
        sizeOfPdfFile = 100;
    }
    auto sizeInBytes = sizeof(Allocation) + sizeOfPdfFile * 2;
    auto bufferStart = (char *)malloc(sizeInBytes);
    ASSERT(bufferStart != nullptr);

    currentAllocation                     = reinterpret_cast<Allocation *>(bufferStart);
    currentAllocation->sizeInBytes        = sizeInBytes;
    currentAllocation->bufferStart        = bufferStart;
    currentAllocation->bufferPosition     = bufferStart + sizeof(Allocation);
    currentAllocation->previousAllocation = nullptr;
}

void Allocator::extend(size_t size) {
    auto sizeInBytes = sizeof(Allocation) + size;
    auto bufferStart = (char *)malloc(sizeInBytes);
    ASSERT(bufferStart != nullptr);

    auto allocation                = reinterpret_cast<Allocation *>(bufferStart);
    allocation->sizeInBytes        = sizeInBytes;
    allocation->bufferStart        = bufferStart;
    allocation->bufferPosition     = bufferStart + sizeof(Allocation);
    allocation->previousAllocation = currentAllocation;
    currentAllocation              = allocation;
}

void Allocator::clear_current_allocation() const {
    currentAllocation->bufferPosition = currentAllocation->bufferStart + sizeof(Allocation);
}

char *Allocator::allocate_chunk(size_t sizeInBytes) {
    ASSERT(currentAllocation->bufferStart != nullptr);
    while (currentAllocation->bufferPosition + sizeInBytes >
        currentAllocation->bufferStart + currentAllocation->sizeInBytes) {
        spdlog::trace("Failed to fit object of size {} bytes into the existing memory, allocating more", sizeInBytes);

        auto newAllocationSize = currentAllocation->sizeInBytes * 2;
        extend(newAllocationSize);
    }

    auto result = currentAllocation->bufferPosition;
    currentAllocation->bufferPosition += sizeInBytes;
    return result;
}

void Allocator::for_each_allocation(const std::function<util::ForEachResult(Allocation &)> &func) const {
    auto allocation = currentAllocation;
    while (allocation != nullptr) {
        auto prev = allocation->previousAllocation;
        func(*allocation);
        allocation = prev;
    }
}

size_t Allocator::total_bytes_allocated() const {
    size_t result = 0;
    for_each_allocation([&result](Allocation &allocation) {
        result += allocation.sizeInBytes;
        return util::ForEachResult::CONTINUE;
    });
    return result;
}

size_t Allocator::num_allocations() const {
    size_t result = 0;
    for_each_allocation([&result](Allocation & /*allocation*/) {
        result++;
        return util::ForEachResult::CONTINUE;
    });
    return result;
}

size_t Allocator::total_bytes_used() const {
    size_t result = 0;
    for_each_allocation([&result](Allocation &allocation) {
        result += allocation.bufferPosition - allocation.bufferStart;
        return util::ForEachResult::CONTINUE;
    });
    return result;
}

} // namespace pdf
