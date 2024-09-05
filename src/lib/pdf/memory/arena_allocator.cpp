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
    const auto ptr = (uint8_t *)mmap(nullptr, sizeInBytes, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ptr != MAP_FAILED) {
        return PtrResult::ok(ptr);
    }

    switch (errno) {
    case EACCES:
        // A file descriptor refers to a non-regular file. Or MAP_PRIVATE was requested, but fd is not open for reading.
        // Or MAP_SHARED was requested and PROT_WRITE is set, but fd is not open in read/write (O_RDWR) mode. Or
        // PROT_WRITE is set, but the file is append-only. EAGAIN The file has been locked, or too much memory has been
        // locked (see setrlimit(2)).
        return PtrResult::error("EACCES: invalid access parameters");
    case EBADF:
        // fd is not a valid file descriptor (and MAP_ANONYMOUS was not set).
        return PtrResult ::error("EBADF: fd is not a valid file descriptor (and MAP_ANONYMOUS was not set).");
    case EINVAL:
        // We don't like addr, length, or offset (e.g., they are too large, or not aligned on a page boundary).
        // or
        // (since Linux 2.6.12) length was 0.
        // or
        // flags contained neither MAP_PRIVATE or MAP_SHARED, or contained both of these values.
        return PtrResult ::error("EINVAL: invalid parameter supplied");
    case ENFILE:
        // The system limit on the total number of open files has been reached.
        return PtrResult ::error("ENFILE: The system limit on the total number of open files has been reached.");
    case ENODEV:
        // The underlying file system of the specified file does not support memory mapping.
        return PtrResult ::error(
              "ENODEV: The underlying file system of the specified file does not support memory mapping.");
    case ENOMEM:
        // No memory is available, or the process's maximum number of mappings would have been exceeded.
        return PtrResult ::error(
              "ENOMEM: No memory is available, or the process's maximum number of mappings would have been exceeded.");
    case EPERM:
        // The prot argument asks for PROT_EXEC but the mapped area belongs to a file on a file system that was mounted
        // no-exec. ETXTBSY MAP_DENYWRITE was set but the object specified by fd is open for writing. EOVERFLOW On
        // 32-bit architecture together with the large file extension (i.e., using 64-bit off_t): the number of pages
        // used for length plus number of pages used for offset would overflow unsigned long (32 bits).
        return PtrResult ::error("EPERM: invalid permissions");
    default:
        return PtrResult::error("failed to reserve memory: {}", errno);
    }
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

    switch (errno) {
    case EACCES:
        // The memory cannot be given the specified access. This can happen, for example, if you mmap(2) a file to which
        // you have read-only access, then ask mprotect() to mark it PROT_WRITE.
        return Result::error("EACCES");
    case EINVAL:
        // addr is not a valid pointer, or not a multiple of the system page size.
        return Result::error("EINVAL: addr is not a valid pointer, or not a multiple of the system page size.");
    case ENOMEM:
        // Internal kernel structures could not be allocated.
        // or
        // Addresses in the range [addr, addr+len-1] are invalid for the address space of the process, or specify one or
        // more pages that are not mapped. (Before kernel 2.4.19, the error EFAULT was incorrectly produced for these
        // cases.)
        return Result::error("ENOMEM: Internal kernel structures could not be allocated.");
    }

    return Result::error("failed to reserve memory of size {}", sizeInBytes);
}
#endif

ValueResult<Arena> Arena::create() {
    constexpr size_t GB = 1024 * 1024 * 1024;
    return create(128 * GB);
}

ValueResult<Arena> Arena::create(size_t maximumSizeInBytes, size_t page_size_in_bytes) {
    auto result = ReserveAddressRange(maximumSizeInBytes);
    if (result.has_error()) {
        return ValueResult<Arena>::error(result.message());
    }
    return ValueResult<Arena>::ok(result.value(), maximumSizeInBytes, page_size_in_bytes);
}

Arena::Arena(uint8_t *_buffer, const size_t _maximumSizeInBytes, const size_t _pageSize) {
    page_size_in_bytes     = _pageSize;
    virtual_size_in_bytes  = _maximumSizeInBytes;
    buffer_start           = _buffer;
    buffer_position        = buffer_start;
    reserved_size_in_bytes = 0;
}

Arena::Arena(Arena &&other) {
    buffer_start           = other.buffer_start;
    buffer_position        = other.buffer_position;
    virtual_size_in_bytes  = other.virtual_size_in_bytes;
    reserved_size_in_bytes = other.reserved_size_in_bytes;
    page_size_in_bytes     = other.page_size_in_bytes;

    other.buffer_start           = nullptr;
    other.buffer_position        = nullptr;
    other.virtual_size_in_bytes  = 0;
    other.reserved_size_in_bytes = 0;
    other.page_size_in_bytes     = 0;
}

Arena &Arena::operator=(Arena &&other) {
    buffer_start           = other.buffer_start;
    buffer_position        = other.buffer_position;
    virtual_size_in_bytes  = other.virtual_size_in_bytes;
    reserved_size_in_bytes = other.reserved_size_in_bytes;
    page_size_in_bytes     = other.page_size_in_bytes;

    other.buffer_start           = nullptr;
    other.buffer_position        = nullptr;
    other.virtual_size_in_bytes  = 0;
    other.reserved_size_in_bytes = 0;
    other.page_size_in_bytes     = 0;

    return *this;
}

Arena::~Arena() {
    if (buffer_start == nullptr) {
        return;
    }

    ReleaseAddressRange(buffer_start, virtual_size_in_bytes);

    buffer_start           = nullptr;
    buffer_position        = nullptr;
    virtual_size_in_bytes  = 0;
    reserved_size_in_bytes = 0;
    page_size_in_bytes     = 0;
}

uint8_t *Arena::push(size_t allocationSizeInBytes) {
    ASSERT(buffer_start != nullptr);

    if (buffer_start + reserved_size_in_bytes < buffer_position + allocationSizeInBytes) {
        const auto pageCount          = (allocationSizeInBytes / page_size_in_bytes) + 1;
        const auto allocationIncrease = page_size_in_bytes * pageCount;
        ASSERT(allocationIncrease + reserved_size_in_bytes <= virtual_size_in_bytes);

        const auto result = ReserveMemory(buffer_start + reserved_size_in_bytes, allocationIncrease);
        if (result.has_error()) {
            spdlog::error(result.message());
            return nullptr;
        }

        reserved_size_in_bytes += allocationIncrease;
    }

    const auto result = buffer_position;
    buffer_position += allocationSizeInBytes;
    return result;
}

void Arena::pop(size_t allocationSizeInBytes) {
    ASSERT(buffer_start != nullptr);
    buffer_position -= allocationSizeInBytes;
}

void Arena::pop_all() { buffer_position = buffer_start; }

ValueResult<Allocator> Allocator::create() {
    auto internalArenaResult = Arena::create();
    if (internalArenaResult.has_error()) {
        return ValueResult<Allocator>::error("failed to create internal arena: " + internalArenaResult.message());
    }

    auto temporaryArenaResult = Arena::create();
    if (temporaryArenaResult.has_error()) {
        return ValueResult<Allocator>::error("failed to create temporary arena: " + temporaryArenaResult.message());
    }

    auto &internal_arena  = internalArenaResult.value();
    auto &temporary_arena = temporaryArenaResult.value();
    return ValueResult<Allocator>::ok(Allocator(internal_arena, temporary_arena));
}

} // namespace pdf
