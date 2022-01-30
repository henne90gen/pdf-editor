#include <spdlog/fmt/bundled/core.h>
#include <string>
#include <utility>

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

namespace pdf {

enum class ForEachResult {
    CONTINUE,
    BREAK,
};

class Result {
  public:
    template <typename... Args> //
    static Result error(fmt::format_string<Args...> fmt, Args &&...args) {
        return {true, fmt::format(fmt, std::forward<Args>(args)...)};
    }

    static Result ok() { return {false, ""}; }

    template <typename... Args> //
    static Result bool_(bool hasError, fmt::format_string<Args...> fmt, Args &&...args) {
        return {hasError, fmt::format(fmt, std::forward<Args>(args)...)};
    }

    [[nodiscard]] bool has_error() const { return hasError; }
    [[nodiscard]] std::string message() const { return errorMessage; }

  private:
    bool hasError = false;
    std::string errorMessage;

    Result(bool _hasError, std::string _errorMessage) : hasError(_hasError), errorMessage(std::move(_errorMessage)) {}
};

template <typename T> class ValueResult {
  public:
    template <typename... Args> //
    static ValueResult error(fmt::format_string<Args...> fmt, Args &&...args) {
        return {{}, true, fmt::format(fmt, std::forward<Args>(args)...)};
    }

    static ValueResult ok(T v) { return {v, false, ""}; }

    [[nodiscard]] T &value() { return _value; }
    [[nodiscard]] T &value() const { return _value; }
    [[nodiscard]] bool has_error() const { return hasError; }
    [[nodiscard]] std::string message() const { return errorMessage; }

  private:
    T _value;
    bool hasError = false;
    std::string errorMessage;

    ValueResult(T v, bool _hasError, std::string _errorMessage)
        : _value(v), hasError(_hasError), errorMessage(std::move(_errorMessage)) {}
};

} // namespace pdf
