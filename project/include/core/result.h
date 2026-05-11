/**
 * @file core/result.h
 * @brief Lightweight Result<T, E> type for fallible operations (C++20).
 *
 * Roughly mirrors std::expected (C++23). When the project moves to C++23
 * this file can be reduced to `using Result = std::expected<...>`.
 */
#pragma once

#include <new>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>

namespace gn {

/// Default error type — a human-readable string. Replace per-call with a
/// stronger enum when meaningful.
struct Error {
    std::string message;

    Error() = default;
    Error(std::string m) : message(std::move(m)) {}
    Error(const char* m) : message(m) {}

    const std::string& what() const noexcept { return message; }
};

/// A sentinel wrapper that lets callers write `return Err{"oops"};`
template <class E>
struct Err {
    E value;
    explicit Err(E v) : value(std::move(v)) {}
};
Err(const char*) -> Err<Error>;
Err(std::string) -> Err<Error>;

/// Result<T, E> — holds either a T (success) or an E (failure).
/// Special-cased for T = void.
template <class T, class E = Error>
class Result {
public:
    using value_type = T;
    using error_type = E;

    Result(const T& v) : m_data(std::in_place_index<0>, v) {}
    Result(T&& v)      : m_data(std::in_place_index<0>, std::move(v)) {}
    Result(Err<E> e)   : m_data(std::in_place_index<1>, std::move(e.value)) {}

    template <class U = E, std::enable_if_t<std::is_constructible_v<E, U>, int> = 0>
    Result(Err<U> e) : m_data(std::in_place_index<1>, std::move(e.value)) {}

    bool ok()  const noexcept { return m_data.index() == 0; }
    bool err() const noexcept { return m_data.index() == 1; }
    explicit operator bool() const noexcept { return ok(); }

    const T& value() const& { return std::get<0>(m_data); }
    T&       value()      & { return std::get<0>(m_data); }
    T&&      value()     && { return std::move(std::get<0>(m_data)); }

    const E& error() const& { return std::get<1>(m_data); }
    E&       error()      & { return std::get<1>(m_data); }

    // Provide a sensible default-on-error.
    template <class U>
    T value_or(U&& fallback) const& {
        return ok() ? value() : static_cast<T>(std::forward<U>(fallback));
    }

private:
    std::variant<T, E> m_data;
};

/// Specialization for void: success carries no payload.
template <class E>
class Result<void, E> {
public:
    using value_type = void;
    using error_type = E;

    Result() : m_has_error(false) {}
    Result(Err<E> e) : m_has_error(true), m_error(std::move(e.value)) {}

    bool ok()  const noexcept { return !m_has_error; }
    bool err() const noexcept { return m_has_error; }
    explicit operator bool() const noexcept { return ok(); }

    const E& error() const& { return m_error; }
    E&       error()      & { return m_error; }

    static Result success() { return Result(); }

private:
    bool m_has_error;
    E    m_error{};
};

inline Result<void> Ok() { return Result<void>::success(); }

} // namespace gn
