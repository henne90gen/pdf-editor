#pragma once

#ifndef __has_builtin
#define __has_builtin(x) 0 // Compatibility with non-clang compilers.
#endif

#ifdef _MSC_VER
#define ASSERT(x)                                                                                                      \
    if (!(x))                                                                                                          \
    __debugbreak()
#else
#if __has_builtin(__builtin_trap)
#define ASSERT(x)                                                                                                      \
    if (!(x))                                                                                                          \
    __builtin_trap()
#else
#define ASSERT(x)
#endif
#endif

#define TODO(x) printf("TODO: %s\n", x)
