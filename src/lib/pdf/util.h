#pragma once

#ifndef __has_builtin      // Optional of course.
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
#endif
#endif

#ifdef _MSC_VER
#define TODO(x) printf("TODO: %s\n", x)
#else
#if __has_builtin(__builtin_trap)
#define TODO(x) printf("TODO: %s\n", x)
#else
#endif
#endif
