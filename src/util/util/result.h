#pragma once

#include "spdlog/spdlog.h"
#include <string>

namespace util {

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

    // TODO add in-place construction similar to emplace_back
    static ValueResult ok(T v) { return {v, false, ""}; }
    static ValueResult of(Result r) { return {{}, r.has_error(), r.message()}; }

    [[nodiscard]] T &value() { return _value; }
    [[nodiscard]] T &value() const { return _value; }
    [[nodiscard]] bool has_error() const { return hasError; }
    [[nodiscard]] std::string message() const { return errorMessage; }
    [[nodiscard]] Result drop_value() const { return Result::bool_(hasError, "{}", errorMessage); }

  private:
    T _value;
    bool hasError = false;
    std::string errorMessage;

    ValueResult(T v, bool _hasError, std::string _errorMessage)
        : _value(v), hasError(_hasError), errorMessage(std::move(_errorMessage)) {}
};
} // namespace util
