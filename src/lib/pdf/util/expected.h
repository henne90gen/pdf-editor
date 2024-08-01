#pragma once

#include <exception>
#include <type_traits>

template <class T> struct remove_cvref {
    typedef std::remove_cv_t<std::remove_reference_t<T>> type;
};
template <class T> using remove_cvref_t = typename remove_cvref<T>::type;

template <class E> class BadExpectedAccess : public std::exception {
  public:
    explicit BadExpectedAccess(E e) : std::exception(), error_(std::move(e)) {}

    E const &error() const & noexcept { return error_; }
    E &error() & noexcept { return error_; }
    E const &&error() const && noexcept { return error_; }
    E &&error() && noexcept { return error_; }

  private:
    E error_;
};
template <> class BadExpectedAccess<void>;

template <class E> class Unexpected {
  public:
    using error_type = E;

    constexpr Unexpected(Unexpected<E> const &) = default;
    constexpr Unexpected(Unexpected<E> &&)      = default;
    constexpr explicit Unexpected(E &&error) : error_(std::forward<E>(error)) {}

    constexpr Unexpected<E> &operator=(Unexpected<E> const &) & = default;

    constexpr Unexpected<E> &operator=(Unexpected<E> &&) && = default;

    constexpr const E &error() const & noexcept { return error_; }
    constexpr E &error() & noexcept { return error_; }
    constexpr const E &&error() const && noexcept { return std::move(error_); }
    constexpr E &&error() && noexcept { return std::move(error_); }

  private:
    E error_;
};

template <class E> Unexpected<remove_cvref_t<E>> unexpected(E &&e) {
    return Unexpected<remove_cvref_t<E>>(std::forward<remove_cvref_t<E>>(const_cast<remove_cvref_t<E> &>(e)));
}

template <class T, class E> class Expected {
  public:
    using value_t      = T;
    using error_t      = E;
    using unexpected_t = Unexpected<E>;
    using self_t       = Expected<value_t, error_t>;

    Expected(Unexpected<E> &&error) : has_value_(false), unexpected_(std::forward<Unexpected<E>>(error)) {}
    Expected(T const &value) : has_value_(true), value_(value) {
        static_assert(std::is_copy_constructible<T>::value, "not copy constructable");
    }
    Expected(T &&value) : has_value_(true), value_(std::forward<T>(value)) {}
    template <class U> Expected(U const &value) : has_value_(true), value_(value) {}
    Expected(Expected const &other) : has_value_(other) {
        if (has_value()) {
            value_ = other.value_;
        } else {
            unexpected_ = other.unexpected_;
        }
    }
    Expected(Expected &&other) : has_value_(other.has_value_) {
        if (has_value()) {
            std::swap(value_, other.value_);
        } else {
            unexpected_ = std::move(other.unexpected_);
        }
    }

    ~Expected() {
        if (has_value()) {
            value_.~value_t();
        } else {
            unexpected_.~unexpected_t();
        }
    }

    constexpr bool has_value() const noexcept { return has_value_; }

    constexpr T &value() & {
        if (!has_value()) {
            throw BadExpectedAccess<E>{error()};
        }
        return value_;
    }
    constexpr T const &value() const & {
        if (!has_value()) {
            throw BadExpectedAccess<E>{error()};
        }
        return value_;
    }
    constexpr T &&value() && {
        if (!has_value()) {
            throw BadExpectedAccess<E>{std::move(error())};
        }
        return std::move(value_);
    }
    constexpr T const &&value() const && {
        if (!has_value()) {
            throw BadExpectedAccess<E>{std::move(error())};
        }
        return std::move(value_);
    }

    template <class Func> self_t &on_value(Func on_success) {
        if (has_value()) {
            on_success(value_);
        }
        return *this;
    }

    template <class Func> self_t &on_error(Func on_fail) {
        if (not has_value()) {
            on_fail(unexpected_.error());
        }
        return *this;
    }

    template <class U> constexpr T value_or(U &&default_value) const & {
        return bool(*this) ? value_ : static_cast<T>(default_value);
    }
    template <class U> constexpr T value_or(U &&default_value) const && {
        return bool(*this) ? value_ : static_cast<T>(default_value);
    }

    constexpr E &error() & noexcept { return unexpected_.error(); }
    constexpr E const &error() const & noexcept { return unexpected_.error(); }
    constexpr E &&error() && noexcept { return std::move(unexpected_.unexpected); }
    constexpr E const &&error() const && noexcept { return std::move(unexpected_.unexpected); }

    constexpr explicit operator bool() const noexcept { return has_value(); }

    constexpr T &operator*() { return value(); }

    constexpr T const &operator*() const { return value(); }

    constexpr T *operator->() { return &value(); }

    constexpr T const *operator->() const { return &value(); }

  private:
    bool has_value_;
    union {
        value_t value_;
        unexpected_t unexpected_;
    };
};

template <class E> class Expected<void, E> {
  public:
    using value_t                   = void;
    using error_t                   = E;
    using unexpected_t              = Unexpected<E>;
    template <class U> using rebind = Expected<U, error_t>;
    using self_t                    = Expected<value_t, error_t>;

    Expected() : has_value_(true) {}
    Expected(Unexpected<E> &&error) : has_value_(false), unexpected_(std::forward<Unexpected<E>>(error)) {}

    ~Expected() {
        if (not has_value()) {
            unexpected_.~unexpected_type();
        }
    }

    constexpr bool has_value() const noexcept { return has_value_; }

    template <class Func> self_t &on_value(Func on_success) {
        if (has_value()) {
            on_success();
        }
        return *this;
    }

    template <class Func> self_t &on_error(Func on_fail) {
        if (not has_value()) {
            on_fail(unexpected_);
        }
        return *this;
    }

    constexpr E &error() & noexcept { return unexpected_.unexpected; }
    constexpr E const &error() const & noexcept { return unexpected_.unexpected; }
    constexpr E &&error() && noexcept { return std::move(unexpected_.error()); }
    constexpr E const &&error() const && noexcept { return std::move(unexpected_.error()); }

    constexpr explicit operator bool() const noexcept { return has_value(); }

  private:
    bool has_value_;
    union {
        unexpected_t unexpected_;
    };
};
