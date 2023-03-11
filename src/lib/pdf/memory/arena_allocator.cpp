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
    if (ptr != MAP_FAILED) {
        return PtrResult::ok(ptr);
    }

    /*
EACCES
    A file descriptor refers to a non-regular file. Or MAP_PRIVATE was requested, but fd is not open for reading. Or
    MAP_SHARED was requested and PROT_WRITE is set, but fd is not open in read/write (O_RDWR) mode. Or PROT_WRITE is
    set, but the file is append-only. EAGAIN The file has been locked, or too much memory has been locked (see
    setrlimit(2)).

EBADF
    fd is not a valid file descriptor (and MAP_ANONYMOUS was not set).

EINVAL
    We don't like addr, length, or offset (e.g., they are too large, or not aligned on a page boundary).

EINVAL
    (since Linux 2.6.12) length was 0.

EINVAL
    flags contained neither MAP_PRIVATE or MAP_SHARED, or contained both of these values.

ENFILE
    The system limit on the total number of open files has been reached.

ENODEV
    The underlying file system of the specified file does not support memory mapping.

ENOMEM
    No memory is available, or the process's maximum number of mappings would have been exceeded.

EPERM
    The prot argument asks for PROT_EXEC but the mapped area belongs to a file on a file system that was mounted
    no-exec. ETXTBSY MAP_DENYWRITE was set but the object specified by fd is open for writing. EOVERFLOW On 32-bit
    architecture together with the large file extension (i.e., using 64-bit off_t): the number of pages used for length
    plus number of pages used for offset would overflow unsigned long (32 bits).
     */
    return PtrResult::error("failed to reserve memory");
};

Result ReleaseAddressRange(uint8_t *buffer, size_t sizeInBytes) {
    // TODO check for error on munmap
    munmap(buffer, sizeInBytes);
    return Result::ok();
}

Result ReserveMemory(uint8_t *buffer, size_t sizeInBytes) {
    const auto err = mprotect(buffer, sizeInBytes, PROT_READ | PROT_WRITE);
    if (err == 0) {
        return Result::ok();
    }

    /*
     EACCES
        The memory cannot be given the specified access. This can happen, for example, if you mmap(2) a file to
    which you have read-only access, then ask mprotect() to mark it PROT_WRITE.

    EINVAL
        addr is not a valid pointer, or not a multiple of the system page size.

    ENOMEM
        Internal kernel structures could not be allocated.

    ENOMEM
        Addresses in the range [addr, addr+len-1] are invalid for the address space of the process, or specify one
    or more pages that are not mapped. (Before kernel 2.4.19, the error EFAULT was incorrectly produced for these
    cases.)
     */
    switch (errno) {
    case EACCES:
        return Result::error("EACCES");
    case EINVAL:
        return Result::error("EINVAL: addr is not a valid pointer, or not a multiple of the system page size.");
    case ENOMEM:
        return Result::error("ENOMEM: Internal kernel structures could not be allocated.");
    }

    return Result::error("failed to reserve memory of size {}", sizeInBytes);
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
    return ValueResult<Arena>::ok(result.value(), maximumSizeInBytes, pageSize);
}

Arena::Arena(uint8_t *_buffer, const size_t _maximumSizeInBytes, const size_t _pageSize) {
    pageSize            = _pageSize;
    virtualSizeInBytes  = _maximumSizeInBytes;
    bufferStart         = _buffer;
    bufferPosition      = bufferStart;
    reservedSizeInBytes = 0;
}

void Arena::destroy() { ReleaseAddressRange(bufferStart, virtualSizeInBytes); }

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

ValueResult<Allocator> Allocator::create() {
    auto internalArenaResult = Arena::create();
    if (internalArenaResult.has_error()) {
        return ValueResult<Allocator>::of(internalArenaResult.drop_value());
    }

    auto temporaryArenaResult = Arena::create();
    if (temporaryArenaResult.has_error()) {
        return ValueResult<Allocator>::of(temporaryArenaResult.drop_value());
    }

    auto internalArena  = internalArenaResult.value();
    auto temporaryArena = temporaryArenaResult.value();
    return ValueResult<Allocator>::ok(Allocator(internalArena, temporaryArena));
}

} // namespace pdf
