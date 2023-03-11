#pragma once

#include <spdlog/spdlog.h>
#include <string>

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
    static Result from_bool(bool hasError, fmt::format_string<Args...> fmt, Args &&...args) {
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

    template <typename... Args> //
    static ValueResult<T> ok(Args &&...args) {
        return {T(std::forward<Args>(args)...), false, ""};
    }
    static ValueResult<T> ok(T v) { return {v, false, ""}; }
    static ValueResult<T> of(const Result &r) { return {{}, r.has_error(), r.message()}; }

    [[nodiscard]] T &value() { return _value; }
    [[nodiscard]] T &value() const { return _value; }
    [[nodiscard]] bool has_error() const { return hasError; }
    [[nodiscard]] std::string message() const { return errorMessage; }
    [[nodiscard]] Result drop_value() const { return Result::from_bool(hasError, "{}", errorMessage); }

    ~ValueResult() = default;

  private:
    T _value;
    bool hasError = false;
    std::string errorMessage;

    ValueResult() = default;
    ValueResult(T v, bool _hasError, std::string _errorMessage)
        : _value(v), hasError(_hasError), errorMessage(std::move(_errorMessage)) {}
};

} // namespace pdf
