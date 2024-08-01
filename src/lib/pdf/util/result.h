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
    static ValueResult<T> error(fmt::format_string<Args...> fmt, Args &&...args) {
        return {true, fmt::format(fmt, std::forward<Args>(args)...)};
    }
    static ValueResult<T> error(std::string message) { return {true, message}; }

    template <typename... Args> //
    static ValueResult<T> ok(Args &&...args) {
        return {T(std::forward<Args>(args)...), false};
    }
    static ValueResult<T> ok(T v) { return {std::move(v), false}; }

    [[nodiscard]] T &value() { return _value; }
    [[nodiscard]] T &value() const { return _value; }
    [[nodiscard]] bool has_error() const { return hasError; }
    [[nodiscard]] std::string message() const { return errorMessage; }
    [[nodiscard]] Result drop_value() const { return Result::from_bool(hasError, "{}", errorMessage); }

    ~ValueResult() {
        if (!has_error()) {
            _value.~T();
        } else {
            typedef std::string string_alias;
            errorMessage.~string_alias();
        }
    }

  private:
    bool hasError = false;
    union {
        T _value;
        std::string errorMessage;
    };

    ValueResult() = default;
    ValueResult(T v, bool _hasError) : hasError(_hasError), _value(std::move(v)) {}
    ValueResult(bool _hasError, std::string _errorMessage)
        : hasError(_hasError), errorMessage(std::move(_errorMessage)) {}
};

} // namespace pdf
